#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd

from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score


# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parents[2]

DATASET = (
    REPO_ROOT.parent
    / "data"
    / "processed"
    / "loop_unroll"
    / "loop_unroll.csv"
)

MODEL_DIR = REPO_ROOT / "models"
MODEL_PATH = MODEL_DIR / "loop_unroll_random_forest.joblib"


# ---------------------------------------------------------------------------
# Features
# ---------------------------------------------------------------------------

FEATURES = [
    "unroll_factor",
    "loop_depth",
    "is_innermost",
    "has_parent_loop",
    "num_subloops",
    "num_basic_blocks",
    "num_instructions",
    "num_terminator_instructions",
    "num_loads",
    "num_stores",
    "num_integer_ops",
    "num_float_ops",
    "num_branches",
    "num_conditional_branches",
    "num_calls",
    "trip_count",
    "memory_op_ratio",
    "control_overhead_ratio",
    "arithmetic_intensity",
]


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("=" * 70)
    print("FINAL LOOP UNROLL PROFITABILITY MODEL")
    print("=" * 70)

    print(f"Dataset: {DATASET}")
    print(f"Model:   {MODEL_PATH}")

    # Load dataset
    df = pd.read_csv(DATASET)

    X = df[FEATURES]
    y = df["profitable"]

    print()
    print("Dataset")
    print("-" * 70)
    print(f"Rows:             {len(df)}")
    print(f"Features:         {len(FEATURES)}")
    print(f"Profitable:       {(y == 1).sum()}")
    print(f"Unprofitable:     {(y == 0).sum()}")

    # Train final model on the complete dataset.
    model = RandomForestClassifier(
        n_estimators=200,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
    )

    print()
    print("Training Random Forest on all samples...")
    model.fit(X, y)

    # Training accuracy is only a sanity check.
    training_predictions = model.predict(X)
    training_accuracy = accuracy_score(y, training_predictions)

    print()
    print("Training sanity check")
    print("-" * 70)
    print(f"Training accuracy: {training_accuracy:.4f}")
    print("(This is NOT the held-out model performance.)")

    # Save model and feature ordering together.
    MODEL_DIR.mkdir(parents=True, exist_ok=True)

    artifact = {
        "model": model,
        "features": FEATURES,
    }

    joblib.dump(artifact, MODEL_PATH)

    print()
    print("Model saved")
    print("-" * 70)
    print(MODEL_PATH)
    print()
    print("Done.")


if __name__ == "__main__":
    main()