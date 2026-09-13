#!/usr/bin/env python3

import pandas as pd

from sklearn.linear_model import LogisticRegression
from sklearn.metrics import (
    accuracy_score,
    precision_score,
    recall_score,
    f1_score,
    confusion_matrix,
)
from sklearn.model_selection import StratifiedGroupKFold
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler


DATASET = (
    "./data/processed/loop_unroll/loop_unroll.csv"
)


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


GROUP_COLUMNS = [
    "benchmark",
    "function",
    "loop_id",
]


def main():

    # ---------------------------------------------------------
    # 1. Load dataset
    # ---------------------------------------------------------

    df = pd.read_csv(DATASET)

    print("=" * 70)
    print("DATASET")
    print("=" * 70)

    print("Rows:", len(df))
    print("Features:", len(FEATURES))
    print("Unique loops:", df[GROUP_COLUMNS].drop_duplicates().shape[0])

    # ---------------------------------------------------------
    # 2. Prepare X and y
    # ---------------------------------------------------------

    X = df[FEATURES]
    y = df["profitable"]

    groups = (
        df[GROUP_COLUMNS]
        .astype(str)
        .agg("_".join, axis=1)
    )

    # ---------------------------------------------------------
    # 3. Build model
    # ---------------------------------------------------------

    model = Pipeline([
        ("scaler", StandardScaler()),
        (
            "classifier",
            LogisticRegression(
                max_iter=1000,
                random_state=42,
            ),
        ),
    ])

    # ---------------------------------------------------------
    # 4. Grouped cross-validation
    # ---------------------------------------------------------

    cv = StratifiedGroupKFold(
        n_splits=5,
        shuffle=True,
        random_state=42,
    )

    all_true = []
    all_pred = []

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

        model.fit(X_train, y_train)

        predictions = model.predict(X_test)

        all_true.extend(y_test)
        all_pred.extend(predictions)

        print(
            f"Fold {fold}: "
            f"train={len(train_idx):3} "
            f"test={len(test_idx):3}"
        )

    # ---------------------------------------------------------
    # 5. Overall evaluation
    # ---------------------------------------------------------

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

    print("\n" + "=" * 70)
    print("LOGISTIC REGRESSION RESULTS")
    print("=" * 70)

    print(f"Accuracy : {accuracy:.4f}")
    print(f"Precision: {precision:.4f}")
    print(f"Recall   : {recall:.4f}")
    print(f"F1       : {f1:.4f}")

    print("\nConfusion matrix:")
    print(cm)


if __name__ == "__main__":
    main()