import duckdb
import struct
import torch
import os

# conn = duckdb.connect()

# # Query only the first 20 million rows straight into an Arrow table or file
# df = conn.execute("""
#     SELECT fen, cp, mate
#     FROM 'hf://datasets/Lichess/chess-position-evaluations/data/*.parquet'
#     LIMIT 86000000
# """).fetch_arrow_table()

# # Save locally to your own lightweight parquet file
# import pyarrow.parquet as pq
# pq.write_table(df, "first_20m_chess.parquet")


state_dict = torch.load("NNUE.pth", map_location="cpu")
os.makedirs("raw_layers", exist_ok=True)

for key, tensor in state_dict.items():
    if "weight" in key or "bias" in key:
        flat_data = tensor.float().detach().cpu().numpy().flatten()

        safe_filename = f"raw_layers/{key.replace('.', '_')}.bin"

        with open(safe_filename, "wb") as f:
            f.write(struct.pack(f"<{len(flat_data)}f", *flat_data))
            
        print(f"Saved {safe_filename} ({len(flat_data)} floats)")