import pandas as pd

def main():
    df = pd.read_csv('positions.csv')

    df = df.drop(columns=[''])

if "__name__" == "__main__":
    main()