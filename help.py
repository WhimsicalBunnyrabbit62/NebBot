import duckdb

conn = duckdb.connect()

# Query only the first 20 million rows straight into an Arrow table or file
df = conn.execute("""
    SELECT fen, cp, mate
    FROM 'hf://datasets/Lichess/chess-position-evaluations/data/*.parquet'
    LIMIT 20000000
""").fetch_arrow_table()

# Save locally to your own lightweight parquet file
import pyarrow.parquet as pq
pq.write_table(df, "first_20m_chess.parquet")