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

    print("=" * 70)
    print("VECTOR CONFIGURATION ANALYSIS")
    print("=" * 70)

    print()
    print("Overall configuration distribution:")
    print(
        df.groupby(["vf", "if"])["profitable"]
        .agg(["count", "mean", "sum"])
        .rename(columns={"mean": "profit_rate", "sum": "profitable_count"})
        .to_string()
    )

    print()
    print("=" * 70)
    print("PROFITABILITY BY VF")
    print("=" * 70)

    print(
        df.groupby("vf")["profitable"]
        .agg(["count", "mean", "sum"])
        .rename(columns={"mean": "profit_rate", "sum": "profitable_count"})
        .to_string()
    )

    print()
    print("=" * 70)
    print("PROFITABILITY BY IF")
    print("=" * 70)

    print(
        df.groupby("if")["profitable"]
        .agg(["count", "mean", "sum"])
        .rename(columns={"mean": "profit_rate", "sum": "profitable_count"})
        .to_string()
    )

    print()
    print("=" * 70)
    print("CONFIGURATION × TRIP COUNT")
    print("=" * 70)

    df["trip_bucket"] = pd.cut(
        df["trip_count"],
        bins=[0, 1000, 1500, 2000, float("inf")],
        labels=["<=1000", "1001-1500", "1501-2000", ">2000"],
    )

    print(
        df.groupby(
            ["vf", "if", "trip_bucket"],
            observed=True,
        )["profitable"]
        .agg(["count", "mean"])
        .rename(columns={"mean": "profit_rate"})
        .to_string()
    )


if __name__ == "__main__":
    main()
