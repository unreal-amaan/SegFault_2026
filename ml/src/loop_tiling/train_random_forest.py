#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd

from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import (
    accuracy_score,
    balanced_accuracy_score,
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
    / "compiler-cost-model"
    / "data"
    / "processed"
    / "loop_tiling"
    / "tiling_dataset.csv"
)

MODEL_OUTPUT = (
    ROOT
    / "ml"
    / "models"
    / "loop_tiling_random_forest.joblib"
)


# ============================================================
# Features
# ============================================================

# synthetic_speedup is deliberately NOT included.
# It is used to create the profitability label and therefore
# must not be available to the model during prediction.

FEATURES = [
    "loop_depth",
    "is_innermost",
    "num_subloops",

    "trip_count_known",
    "trip_count",
    "max_trip_count_known",
    "max_trip_count",

    "num_instructions",
    "num_loads",
    "num_stores",
    "num_integer_ops",
    "num_float_ops",

    "num_forward_contiguous_loads",
    "num_strided_loads",
    "num_forward_contiguous_stores",
    "num_strided_stores",

    "tile_size",
]


# ============================================================
# Target
# ============================================================

TARGET = "profitable"


# ============================================================
# Grouping
# ============================================================

# The four tile-size rows belonging to the same loop must stay
# in the same fold.
#
# Otherwise the model could see the same loop during training
# and validation, producing an overly optimistic evaluation.

GROUP_COLUMNS = [
    "benchmark",
    "function",
    "loop_id",
]


# ============================================================
# Model
# ============================================================

def create_model():
    return RandomForestClassifier(
        n_estimators=300,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
        max_features="sqrt",
    )


# ============================================================
# Main
# ============================================================

def main():

    print("=" * 70)
    print("LOOP TILING RANDOM FOREST")
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
        TARGET,
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
        f"Unique loops   : "
        f"{df[GROUP_COLUMNS].drop_duplicates().shape[0]}"
    )

    print(
        f"Unique feature vectors: "
        f"{df[FEATURES].drop_duplicates().shape[0]}"
    )

    print("\nTarget distribution:")

    target_distribution = (
        df[TARGET]
        .value_counts()
        .sort_index()
        .rename({
            0: "unprofitable",
            1: "profitable",
        })
    )

    print(target_distribution)

    print("\nTarget percentage:")

    print(
        df[TARGET]
        .value_counts(normalize=True)
        .sort_index()
        .mul(100)
        .round(2)
        .rename({
            0: "unprofitable",
            1: "profitable",
        })
    )

    # --------------------------------------------------------
    # Majority-class baseline
    # --------------------------------------------------------

    majority_class = df[TARGET].mode()[0]

    baseline_predictions = [majority_class] * len(df)

    baseline_accuracy = accuracy_score(
        df[TARGET],
        baseline_predictions,
    )

    baseline_balanced_accuracy = balanced_accuracy_score(
        df[TARGET],
        baseline_predictions,
    )

    print("\n" + "=" * 70)
    print("MAJORITY-CLASS BASELINE")
    print("=" * 70)

    print(
        f"Always predict class {majority_class}:"
    )

    print(
        f"Accuracy          : "
        f"{baseline_accuracy:.4f}"
    )

    print(
        f"Balanced accuracy : "
        f"{baseline_balanced_accuracy:.4f}"
    )

    # --------------------------------------------------------
    # Prepare X / y
    # --------------------------------------------------------

    X = df[FEATURES]
    y = df[TARGET]

    groups = (
        df[GROUP_COLUMNS]
        .astype(str)
        .agg("_".join, axis=1)
    )

    # --------------------------------------------------------
    # Grouped cross-validation
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

        model = create_model()

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

        fold_metrics.append(
            [
                accuracy,
                balanced_accuracy,
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

        train_groups = groups.iloc[train_idx].nunique()
        test_groups = groups.iloc[test_idx].nunique()

        print(
            f"Fold {fold}: "
            f"train_rows={len(train_idx):3} "
            f"test_rows={len(test_idx):3} "
            f"train_loops={train_groups:2} "
            f"test_loops={test_groups:2} "
            f"accuracy={accuracy:.4f} "
            f"balanced_acc={balanced_accuracy:.4f} "
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

    balanced_accuracy = balanced_accuracy_score(
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
            "balanced_accuracy",
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
            f"{metric:20s}: "
            f"{mean:.4f} ± {std:.4f}"
        )

    # --------------------------------------------------------
    # Overall results
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("CROSS-VALIDATION RESULTS")
    print("=" * 70)

    print(f"Accuracy          : {accuracy:.4f}")
    print(f"Balanced accuracy : {balanced_accuracy:.4f}")
    print(f"Precision         : {precision:.4f}")
    print(f"Recall            : {recall:.4f}")
    print(f"F1                : {f1:.4f}")

    print("\nConfusion matrix:")
    print(cm)

    print("\nClass order:")
    print("  [[true 0 predicted 0, true 0 predicted 1],")
    print("   [true 1 predicted 0, true 1 predicted 1]]")

    # --------------------------------------------------------
    # Train final model on complete dataset
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("TRAINING FINAL MODEL")
    print("=" * 70)

    final_model = create_model()

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

    print("\nFeature importances:")

    for _, row in importance.iterrows():

        print(
            f"  {row['feature']:<40} "
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
            "target": TARGET,
            "group_columns": GROUP_COLUMNS,
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