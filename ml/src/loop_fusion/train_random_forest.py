#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd

from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import (
    accuracy_score,
    confusion_matrix,
    f1_score,
    precision_score,
    recall_score,
)
from sklearn.model_selection import StratifiedGroupKFold


# ============================================================
# Paths
# ============================================================

ROOT = Path(__file__).resolve().parents[3]

DATASET = (
    ROOT.parent
    / "compiler-cost-model-data"
    / "data"
    / "loop_fusion"
    / "processed"
    / "loop_fusion_custom.csv"
)

MODEL_OUTPUT = (
    ROOT
    / "ml"
    / "models"
    / "loop_fusion_random_forest.joblib"
)


# ============================================================
# Features
# ============================================================

FEATURES = [
    # "loop1_id",
    # "loop2_id",

    "loop1_trip_count_known",
    "loop1_trip_count",
    "loop1_num_instructions",
    "loop1_num_loads",
    "loop1_num_stores",
    "loop1_num_forward_contiguous_loads",
    "loop1_num_forward_contiguous_stores",
    "loop1_num_integer_ops",
    "loop1_num_float_ops",
    "loop1_num_branches",
    "loop1_num_calls",
    "loop1_memory_op_ratio",
    "loop1_contiguous_memory_ops",

    "loop2_trip_count_known",
    "loop2_trip_count",
    "loop2_num_instructions",
    "loop2_num_loads",
    "loop2_num_stores",
    "loop2_num_forward_contiguous_loads",
    "loop2_num_forward_contiguous_stores",
    "loop2_num_integer_ops",
    "loop2_num_float_ops",
    "loop2_num_branches",
    "loop2_num_calls",
    "loop2_memory_op_ratio",
    "loop2_contiguous_memory_ops",

    "loops_adjacent",
    "trip_count_equal",
    "trip_count_ratio",

    "shared_memory_objects",
    "loop1_stores_read_by_loop2",
    "loop2_stores_read_by_loop1",

    "combined_memory_ops",
]


# ============================================================
# Grouping
# ============================================================

GROUP_COLUMNS = [
    "benchmark",
    "loop1_id",
    "loop2_id",
]


# ============================================================
# Main
# ============================================================

def main():
    print("=" * 70)
    print("LOOP FUSION RANDOM FOREST")
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
    # Validate columns
    # --------------------------------------------------------

    required_columns = FEATURES + [
        "label",
    ] + GROUP_COLUMNS

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
    # Dataset information
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("DATASET")
    print("=" * 70)

    print(f"Rows           : {len(df)}")
    print(f"Features       : {len(FEATURES)}")
    print(
        f"Unique pairs   : "
        f"{df[GROUP_COLUMNS].drop_duplicates().shape[0]}"
    )

    print("\nLabel distribution:")
    print(
        df["label"]
        .value_counts()
        .sort_index()
        .rename({
            0: "unprofitable",
            1: "profitable",
        })
    )

    # --------------------------------------------------------
    # Prepare X / y
    # --------------------------------------------------------

    X = df[FEATURES]
    y = df["label"]

    groups = (
        df[GROUP_COLUMNS]
        .astype(str)
        .agg("_".join, axis=1)
    )

    # --------------------------------------------------------
    # Grouped cross-validation
    #
    # We don't want the same fusion pair appearing in both
    # training and validation data.
    # --------------------------------------------------------

    cv = StratifiedGroupKFold(
        n_splits=5,
        shuffle=True,
        random_state=42,
    )

    all_true = []
    all_pred = []

    fold_metrics = []

    print("\n" + "=" * 70)
    print("5-FOLD GROUPED CROSS-VALIDATION")
    print("=" * 70)

    for fold, (train_idx, test_idx) in enumerate(
        cv.split(X, y, groups),
        start=1,
    ):
        X_train = X.iloc[train_idx]
        X_test = X.iloc[test_idx]

        y_train = y.iloc[train_idx]
        y_test = y.iloc[test_idx]

        model = RandomForestClassifier(
            n_estimators=300,
            random_state=42,
            class_weight="balanced",
            n_jobs=-1,
            max_features="sqrt",
        )

        model.fit(
            X_train,
            y_train,
        )

        predictions = model.predict(
            X_test
        )

        accuracy = accuracy_score(
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

        fold_metrics.append(
            [
                accuracy,
                precision,
                recall,
                f1,
            ]
        )

        all_true.extend(
            y_test.tolist()
        )

        all_pred.extend(
            predictions.tolist()
        )

        print(
            f"Fold {fold}: "
            f"train={len(train_idx):3} "
            f"test={len(test_idx):3} "
            f"accuracy={accuracy:.4f} "
            f"precision={precision:.4f} "
            f"recall={recall:.4f} "
            f"F1={f1:.4f}"
        )

    # --------------------------------------------------------
    # Overall cross-validation metrics
    # --------------------------------------------------------

    accuracy = accuracy_score(
        all_true,
        all_pred,
    )

    precision = precision_score(
        all_true,
        all_pred,
        zero_division=0,
    )

    recall = recall_score(
        all_true,
        all_pred,
        zero_division=0,
    )

    f1 = f1_score(
        all_true,
        all_pred,
        zero_division=0,
    )

    cm = confusion_matrix(
        all_true,
        all_pred,
    )

    fold_metrics = pd.DataFrame(
        fold_metrics,
        columns=[
            "accuracy",
            "precision",
            "recall",
            "f1",
        ],
    )

    # --------------------------------------------------------
    # Per-fold statistics
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("PER-FOLD MEAN ± STD")
    print("=" * 70)

    for metric in fold_metrics.columns:
        mean = fold_metrics[metric].mean()
        std = fold_metrics[metric].std()

        print(
            f"{metric.capitalize():10s}: "
            f"{mean:.4f} ± {std:.4f}"
        )

    # --------------------------------------------------------
    # Overall results
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("CROSS-VALIDATION RESULTS")
    print("=" * 70)

    print(f"Accuracy : {accuracy:.4f}")
    print(f"Precision: {precision:.4f}")
    print(f"Recall   : {recall:.4f}")
    print(f"F1       : {f1:.4f}")

    print("\nConfusion matrix:")
    print(cm)

    # --------------------------------------------------------
    # Train final model on the complete dataset
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("TRAINING FINAL MODEL")
    print("=" * 70)

    final_model = RandomForestClassifier(
        n_estimators=300,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
        max_features="sqrt",
    )

    final_model.fit(
        X,
        y,
    )

    # --------------------------------------------------------
    # Feature importance
    # --------------------------------------------------------

    importance = pd.DataFrame(
        {
            "feature": FEATURES,
            "importance": final_model.feature_importances_,
        }
    ).sort_values(
        "importance",
        ascending=False,
    )

    print("\nTop 15 feature importances:")

    for _, row in importance.head(15).iterrows():
        print(
            f"  {row['feature']:<45} "
            f"{row['importance']:.4f}"
        )

    # --------------------------------------------------------
    # Save model
    # --------------------------------------------------------

    MODEL_OUTPUT.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    joblib.dump(
        {
            "model": final_model,
            "features": FEATURES,
        },
        MODEL_OUTPUT,
    )

    print("\n[+] Final model saved:")
    print(f"    {MODEL_OUTPUT}")

    print("\n" + "=" * 70)
    print("DONE")
    print("=" * 70)


if __name__ == "__main__":
    main()