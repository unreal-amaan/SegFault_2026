#!/usr/bin/env python3

from pathlib import Path
import pandas as pd


ROOT = Path(__file__).resolve().parents[3]

DATASET = (
    ROOT
    / "data"
    / "processed"
    / "loop_vectorization"
    / "vectorization_dataset.csv"
)


def main():
    df = pd.read_csv(DATASET)

    df["group"] = (
        df["benchmark"].astype(str)
        + "::"
        + df["loop_id"].astype(str)
    )

    print("=" * 70)
    print("PROFITABILITY BY LOOP GROUP")
    print("=" * 70)

    groups = (
        df.groupby("group")
        .agg(
            samples=("profitable", "size"),
            profitable=("profitable", "sum"),
            profit_rate=("profitable", "mean"),
        )
        .sort_values("profit_rate")
    )

    print(
        groups.to_string(
            float_format=lambda x: f"{x:.4f}"
        )
    )

    print()
    print("=" * 70)
    print("CONFIGURATION RANGE PER GROUP")
    print("=" * 70)

    for group, g in df.groupby("group"):
        profitable = g[g["profitable"] == 1]
        unprofitable = g[g["profitable"] == 0]

        print()
        print(group)
        print("-" * 50)

        print(
            f"profit rate : {g['profitable'].mean():.4f}"
        )

        print(
            f"profitable configurations : "
            f"{len(profitable)}/{len(g)}"
        )

        if len(profitable) > 0:
            best = profitable.loc[
                profitable["runtime_ms"].idxmin()
            ] if "runtime_ms" in profitable.columns else None

            print("profitable VF/IF:")
            print(
                profitable[["vf", "if"]]
                .to_string(index=False)
            )

        if len(unprofitable) > 0:
            print("unprofitable VF/IF:")
            print(
                unprofitable[["vf", "if"]]
                .to_string(index=False)
            )


if __name__ == "__main__":
    main()
