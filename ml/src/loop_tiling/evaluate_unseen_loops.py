#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd

from sklearn.metrics import (
    accuracy_score,
    balanced_accuracy_score,
    confusion_matrix,
    f1_score,
    precision_score,
    recall_score,
)
from sklearn.model_selection import GroupShuffleSplit


# ============================================================
# Paths
# ============================================================

ROOT = Path(__file__).resolve().parents[3]

DATASET = (
    ROOT.parent
    / "compiler-cost-model-data"
    / "ml"
    / "loop_tiling"
    / "data"
    / "tiling_dataset.csv"
)

MODEL_PATH = (
    ROOT
    / "ml"
    / "models"
    / "loop_tiling_random_forest.joblib"
)


# ============================================================
# Dataset columns
# ============================================================

GROUP_COLUMNS = [
    "benchmark",
    "function",
    "loop_id",
]

TARGET = "profitable"


# ============================================================
# Main
# ============================================================

def main():

    print("=" * 70)
    print("LOOP TILING — UNSEEN LOOP EVALUATION")
    print("=" * 70)

    # --------------------------------------------------------
    # Load dataset
    # --------------------------------------------------------

    print("\n[+] Loading dataset:")
    print(f"    {DATASET}")

    if not DATASET.exists():
        raise FileNotFoundError(
            f"Dataset not found:\n{DATASET}"
        )

    df = pd.read_csv(DATASET)

    print(f"    rows: {len(df)}")

    # --------------------------------------------------------
    # Load model
    # --------------------------------------------------------

    print("\n[+] Loading model:")
    print(f"    {MODEL_PATH}")

    if not MODEL_PATH.exists():
        raise FileNotFoundError(
            f"Model not found:\n{MODEL_PATH}"
        )

    bundle = joblib.load(MODEL_PATH)

    model = bundle["model"]
    features = bundle["features"]

    print(f"    features: {len(features)}")

    # --------------------------------------------------------
    # Validate dataset
    # --------------------------------------------------------

    required_columns = (
        features
        + [TARGET]
        + GROUP_COLUMNS
    )

    missing = [
        column
        for column in required_columns
        if column not in df.columns
    ]

    if missing:
        raise RuntimeError(
            "Dataset is missing required columns:\n"
            + "\n".join(missing)
        )

    # --------------------------------------------------------
    # Build groups
    # --------------------------------------------------------

    groups = (
        df[GROUP_COLUMNS]
        .astype(str)
        .agg("_".join, axis=1)
    )

    X = df[features]
    y = df[TARGET]

    # --------------------------------------------------------
    # Hold out complete loops
    #
    # Every tile-size measurement belonging to a loop stays
    # together.
    # --------------------------------------------------------

    splitter = GroupShuffleSplit(
        n_splits=1,
        test_size=0.20,
        random_state=42,
    )

    train_idx, test_idx = next(
        splitter.split(
            X,
            y,
            groups,
        )
    )

    X_test = X.iloc[test_idx]
    y_test = y.iloc[test_idx]

    test_groups = groups.iloc[test_idx]

    # --------------------------------------------------------
    # Verify no loop leakage
    # --------------------------------------------------------

    train_groups = set(
        groups.iloc[train_idx]
    )

    held_out_groups = set(
        test_groups
    )

    overlap = train_groups & held_out_groups

    print("\n" + "=" * 70)
    print("SPLIT")
    print("=" * 70)

    print(
        f"Training rows     : {len(train_idx)}"
    )

    print(
        f"Test rows         : {len(test_idx)}"
    )

    print(
        f"Training loops    : "
        f"{len(train_groups)}"
    )

    print(
        f"Unseen test loops : "
        f"{len(held_out_groups)}"
    )

    print(
        f"Loop overlap      : "
        f"{len(overlap)}"
    )

    if overlap:
        raise RuntimeError(
            "DATA LEAKAGE: some test loops also "
            "appear in training data."
        )

    # --------------------------------------------------------
    # IMPORTANT:
    #
    # The saved model was trained on the COMPLETE dataset.
    #
    # Therefore, this script cannot use it for a genuine
    # unseen-loop evaluation.
    #
    # We train a fresh model ONLY on the training loops.
    # --------------------------------------------------------

    from sklearn.ensemble import RandomForestClassifier

    evaluation_model = RandomForestClassifier(
        n_estimators=300,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
        max_features="sqrt",
    )

    X_train = X.iloc[train_idx]
    y_train = y.iloc[train_idx]

    evaluation_model.fit(
        X_train,
        y_train,
    )

    # --------------------------------------------------------
    # Predict unseen loops
    # --------------------------------------------------------

    predictions = evaluation_model.predict(
        X_test
    )

    # --------------------------------------------------------
    # Metrics
    # --------------------------------------------------------

    accuracy = accuracy_score(
        y_test,
        predictions,
    )

    balanced_accuracy = balanced_accuracy_score(
        y_test,
        predictions,
    )

    precision = precision_score(
        y_test,
        predictions,
        zero_division=0,
    )

    recall = recall_score(
        y_test,
        predictions,
        zero_division=0,
    )

    f1 = f1_score(
        y_test,
        predictions,
        zero_division=0,
    )

    cm = confusion_matrix(
        y_test,
        predictions,
    )

    # --------------------------------------------------------
    # Results
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("UNSEEN LOOP RESULTS")
    print("=" * 70)

    print(
        f"Accuracy          : "
        f"{accuracy:.4f}"
    )

    print(
        f"Balanced accuracy : "
        f"{balanced_accuracy:.4f}"
    )

    print(
        f"Precision         : "
        f"{precision:.4f}"
    )

    print(
        f"Recall            : "
        f"{recall:.4f}"
    )

    print(
        f"F1                : "
        f"{f1:.4f}"
    )

    print("\nConfusion matrix:")
    print(cm)

    print("\nClass order:")
    print(
        "  [[true 0 predicted 0, "
        "true 0 predicted 1],"
    )
    print(
        "   [true 1 predicted 0, "
        "true 1 predicted 1]]"
    )

    # --------------------------------------------------------
    # Show held-out loops
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("HELD-OUT LOOPS")
    print("=" * 70)

    held_out = (
        df.iloc[test_idx][GROUP_COLUMNS]
        .drop_duplicates()
        .sort_values(
            ["benchmark", "function", "loop_id"]
        )
    )

    print(
        held_out.to_string(index=False)
    )

    print("\n" + "=" * 70)
    print("DONE")
    print("=" * 70)


if __name__ == "__main__":
    main()