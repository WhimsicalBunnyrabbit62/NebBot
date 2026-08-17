import math
import numpy as np
import pandas as pd
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset
from tqdm import tqdm
import polars as pl

FEATURE_COUNT = 12 * 64
MAX_PIECES = 32
CSV_CHUNK_SIZE = 100_000
BATCH_SIZE = 4_096
SCORE_SCALE = 250.0
PIECE_TO_INDEX = {piece: index for index, piece in enumerate("PNBRQKpnbrqk")}

class NN(nn.Module):
    def __init__(self):
        super().__init__()

        self.sharedLayer = nn.Linear(FEATURE_COUNT, 256)
        self.layerOne = nn.Linear(512, 512)
        self.layerTwo = nn.Linear(512, 256)
        self.layerThree = nn.Linear(256, 128)
        self.layerFour = nn.Linear(128, 64)
        self.layerFive = nn.Linear(64, 1)

    def forward(self, white_input, black_input):
        white_features = self.sharedLayer(white_input)
        black_features = self.sharedLayer(black_input)

        combined = torch.cat((white_features, black_features), dim=-1)
        x = F.relu(combined)
        x = self.layerOne(x)
        x = F.relu(x)
        x = self.layerTwo(x)
        x = F.relu(x)
        x = self.layerThree(x)
        x = F.relu(x)
        x = self.layerFour(x)
        x = F.relu(x)
        x = self.layerFive(x)
        return torch.tanh(x)


def encode_fen(fen):
    white = np.full(MAX_PIECES, -1, dtype=np.int16)
    black = np.full(MAX_PIECES, -1, dtype=np.int16)
    count = 0
    fields = fen.split()
    if len(fields) < 2:
        return None
    board, side_to_move = fields[:2]
    turn = side_to_move == "w"
    for row_index, row in enumerate(board.split("/")):
        column = 0
        square = (7 - row_index) * 8
        for char in row:
            if char.isdigit():
                column += int(char)
                continue
            if count >= MAX_PIECES or char not in PIECE_TO_INDEX:
                return None
            piece = PIECE_TO_INDEX[char]
            white[count] = piece * 64 + square + column
            black_piece = piece - 6 if piece >= 6 else piece + 6
            black[count] = black_piece * 64 + ((square + column)^56)
            count += 1
            column += 1

    if turn: return white, black, turn
    else: return black, white, turn


def encode_chunk(fens, scores):
    white = np.full((len(fens), MAX_PIECES), -1, dtype=np.int16)
    black = np.full((len(fens), MAX_PIECES), -1, dtype=np.int16)
    targets = np.empty(len(fens), dtype=np.float32)
    skipped = 0
    output_index = 0

    for fen, score in zip(fens, scores):
        encoded = encode_fen(fen)
        if encoded is None:
            skipped += 1
            continue

        us, them, turn = encoded
        white[output_index], black[output_index] = us, them
        targets[output_index] = score if turn else -score
        output_index += 1

    return white[:output_index], black[:output_index], targets[:output_index], skipped


def normalize_scores(scores):
    return np.tanh(np.asarray(scores, dtype=np.float32) / SCORE_SCALE).astype(np.float32)


def position_chunks():
    for chunk in pd.read_csv("positions.csv", usecols=["fen", "score", "mate"], chunksize=CSV_CHUNK_SIZE):
        score = pd.to_numeric(chunk["score"], errors="coerce")
        mate = pd.to_numeric(chunk["mate"], errors="coerce")
        score = score.mask(score.isna() & mate.gt(0), 3000.0)
        score = score.mask(score.isna() & mate.lt(0), -3000.0)
        valid = chunk["fen"].notna() & score.notna()
        if valid.any():
            yield chunk.loc[valid, "fen"].to_numpy(), normalize_scores(score[valid])

def huggingface_chunks():
    query = pl.scan_parquet("chessy.parquet")
    processed_query = query.select([
        pl.col("fen"),
        pl.col("cp").cast(pl.Float64, strict=False),
        pl.col("mate").cast(pl.Float64, strict=False)
    ]).select([
        pl.col("fen"),
        pl.when(pl.col("cp").is_null() & (pl.col("mate") > 0))
        .then(3000.0)
        .when(pl.col("cp").is_null() & (pl.col("mate") < 0))
        .then(-3000.0)
        .otherwise(pl.col("cp"))
        .alias("score")
    ]).filter(
        pl.col("fen").is_not_null() & pl.col("score").is_not_null()
    )

    for chunk in processed_query.collect_batches(chunk_size=CSV_CHUNK_SIZE, maintain_order=False):
        if len(chunk) > 0:
            fens = chunk["fen"].to_numpy()
            scores = normalize_scores(chunk["score"].to_numpy())

            yield fens, scores

def chess_data_chunks():
    for chunk in pd.read_csv("chessData.csv", usecols=["FEN", "Evaluation"], chunksize=CSV_CHUNK_SIZE):
        evaluation = chunk["Evaluation"].astype("string")
        score = pd.to_numeric(evaluation, errors="coerce")
        is_mate = evaluation.str.contains("#", na=False)
        score = score.mask(is_mate & evaluation.str.contains("-", na=False), -3000.0)
        score = score.mask(is_mate & ~evaluation.str.contains("-", na=False), 3000.0)
        valid = chunk["FEN"].notna() & score.notna()
        if valid.any():
            yield chunk.loc[valid, "FEN"].to_numpy(), normalize_scores(score[valid])


def build_data():
    white_chunks, black_chunks, target_chunks = [], [], []
    skipped_fens = 0
    for label, chunks in (("positions", position_chunks()), ("chessData", chess_data_chunks()), ("chessyP", huggingface_chunks())):
        for fens, scores in tqdm(chunks, desc=f"encoding {label}", unit="chunk"):
            white, black, targets, skipped = encode_chunk(fens, scores)
            white_chunks.append(white)
            black_chunks.append(black)
            target_chunks.append(targets)
            skipped_fens += skipped

    np.save("white_in.npy", np.concatenate(white_chunks))
    np.save("black_in.npy", np.concatenate(black_chunks))
    np.save("targets.npy", np.concatenate(target_chunks))
    print(f"Saved {sum(map(len, target_chunks)):,} positions; skipped {skipped_fens:,} invalid FENs.")

def indices_to_features(indices, device):
    indices = indices.to(device, non_blocking=True)
    features = torch.zeros((indices.size(0), FEATURE_COUNT), device=device)
    valid = indices >= 0
    rows = torch.arange(indices.size(0), device=device).unsqueeze(1).expand_as(indices)
    features[rows[valid], indices[valid].long()] = 1.0
    return features


def evaluate(model, dataloader, device):
    model.eval()
    total_loss = 0.0
    with torch.no_grad():
        for white_indices, black_indices, score in dataloader:
            prediction = model(
                indices_to_features(white_indices, device),
                indices_to_features(black_indices, device),
            )
            total_loss += F.mse_loss(
                prediction.squeeze(1), score.to(device, non_blocking=True)
            ).item()
    return total_loss / len(dataloader)


def train(model):
    split = 40_000_000
    splitEnd = 10_000_000
    total = split + splitEnd
    white_inputs = torch.from_numpy(np.load("white_in.npy", mmap_mode="r")[:total].copy())
    black_inputs = torch.from_numpy(np.load("black_in.npy", mmap_mode="r")[:total].copy())
    target_scores = torch.from_numpy(np.load("targets.npy", mmap_mode="r")[:total].copy()).float()
    train_dataset = TensorDataset(white_inputs[:split], black_inputs[:split], target_scores[:split])
    val_dataset = TensorDataset(white_inputs[split:split+splitEnd], black_inputs[split:split+splitEnd], target_scores[split:split+splitEnd])
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    pin_memory = device.type == "cuda"
    train_dataloader = DataLoader(train_dataset, batch_size=BATCH_SIZE, shuffle=True, pin_memory=pin_memory, num_workers=4)
    val_dataloader = DataLoader(val_dataset, batch_size=BATCH_SIZE, pin_memory=pin_memory, num_workers=4)
    model.to(device)
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    best_val_loss = math.inf

    for epoch in range(32):
        if epoch == 24:
            for param_group in optimizer.param_groups:
                param_group['lr'] = 0.0001
        model.train()
        total_loss = 0.0
        for white_indices, black_indices, score in tqdm(train_dataloader, desc=f"epoch {epoch + 1}"):
            prediction = model(
                indices_to_features(white_indices, device),
                indices_to_features(black_indices, device),
            )
            loss = F.mse_loss(prediction.squeeze(1), score.to(device, non_blocking=True))
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            total_loss += loss.item()

        print(f"Average loss for epoch {epoch + 1}: {total_loss / len(train_dataloader):.6f}")
        val_loss = evaluate(model, val_dataloader, device)
        if val_loss < best_val_loss:
            best_val_loss = val_loss
            torch.save(model.state_dict(), "NNUE.pth")
        print(f"Validation loss: {val_loss:.6f}")

def main():
    #build_data()
    train(NN())

if __name__ == "__main__":
    main()
