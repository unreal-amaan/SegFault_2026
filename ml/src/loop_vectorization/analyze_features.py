#!/usr/bin/env python3

from pathlib import Path

import pandas as pd

from sklearn.ensemble import RandomForestClassifier


ROOT = Path(__file__).resolve().parents[3]

DATASET = (
    ROOT
    / "data"
    / "processed"
    / "loop_vectorization"
    / "vectorization_dataset.csv"
)

FEATURES = [
    "loop_depth",
    "num_instructions",
    "num_phi_nodes",
    "num_loads",
    "num_stores",
    "num_forward_contiguous_loads",
    "num_strided_loads",
    "num_forward_contiguous_stores",
    "num_float_ops",
    "has_reduction",
    "num_reductions",
    "num_calls",
    "trip_count",
    "memory_op_ratio",
    "control_overhead_ratio",
    "arithmetic_intensity",
    "vf",
    "if",
]


def main():
    df = pd.read_csv(DATASET)

    X = df[FEATURES]
    y = df["profitable"].astype(int)

    model = RandomForestClassifier(
        n_estimators=500,
        min_samples_leaf=2,
        random_state=42,
        n_jobs=-1,
    )

    model.fit(X, y)

    importance = pd.Series(
        model.feature_importances_,
        index=FEATURES,
    ).sort_values(ascending=False)

    print("=" * 70)
    print("RANDOM FOREST FEATURE IMPORTANCE")
    print("=" * 70)

    for feature, value in importance.items():
        print(f"{feature:35s} {value:.4f}")

    print()
    print("=" * 70)
    print("TOP FEATURES")
    print("=" * 70)

    for feature, value in importance.head(10).items():
        print(f"{feature:35s} {value:.4f}")


if __name__ == "__main__":
    main()
