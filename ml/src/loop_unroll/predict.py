#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd

from sklearn.metrics import (
    accuracy_score,
    precision_score,
    recall_score,
    f1_score,
    confusion_matrix,
)


# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parents[2]

MODEL_PATH = (
    REPO_ROOT
    / "models"
    / "loop_unroll_random_forest.joblib"
)

DATASET = (
    REPO_ROOT.parent
    / "data"
    / "processed"
    / "loop_unroll"
    / "loop_unroll.csv"
)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("=" * 70)
    print("LOOP UNROLL PROFITABILITY INFERENCE")
    print("=" * 70)

    artifact = joblib.load(MODEL_PATH)

    model = artifact["model"]
    features = artifact["features"]

    df = pd.read_csv(DATASET)

    X = df[features]
    y = df["profitable"]

    predictions = model.predict(X)

    accuracy = accuracy_score(y, predictions)
    precision = precision_score(y, predictions, zero_division=0)
    recall = recall_score(y, predictions, zero_division=0)
    f1 = f1_score(y, predictions, zero_division=0)
    cm = confusion_matrix(y, predictions)

    print()
    print("Dataset")
    print("-" * 70)
    print(f"Candidates: {len(df)}")
    print(f"Features:   {len(features)}")

    print()
    print("Inference results")
    print("-" * 70)
    print(f"Accuracy : {accuracy:.4f}")
    print(f"Precision: {precision:.4f}")
    print(f"Recall   : {recall:.4f}")
    print(f"F1       : {f1:.4f}")

    print()
    print("Confusion matrix:")
    print(cm)

    print()
    print("NOTE:")
    print("This evaluates the model on the same data used for training.")
    print("It is a sanity check, NOT the final performance estimate.")

    print()
    print("Done.")


if __name__ == "__main__":
    main()