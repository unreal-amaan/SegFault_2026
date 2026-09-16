#!/usr/bin/env python3

from pathlib import Path

import pandas as pd
from sklearn.metrics import (
    accuracy_score,
    balanced_accuracy_score,
    confusion_matrix,
    classification_report,
)
from sklearn.model_selection import LeaveOneGroupOut


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
    print("CONFIGURATION-ONLY BASELINE")
    print("=" * 70)

    print(f"Dataset : {DATASET}")
    print(f"Samples : {len(df)}")

    # ------------------------------------------------------------
    # Learn the majority label for each (VF, IF) configuration.
    #
    # IMPORTANT:
    # This must be learned separately inside every training fold.
    # Otherwise we would leak information from the test loops.
    # ------------------------------------------------------------

    X = df[["vf", "if"]]
    y = df["profitable"].astype(int)

    groups = (
        df["benchmark"].astype(str)
        + "::"
        + df["loop_id"].astype(str)
    )

    logo = LeaveOneGroupOut()

    all_y_true = []
    all_y_pred = []

    for fold, (train_idx, test_idx) in enumerate(
        logo.split(X, y, groups),
        start=1,
    ):
        train = df.iloc[train_idx]
        test = df.iloc[test_idx]

        # Majority label for each configuration
        config_majority = (
            train.groupby(["vf", "if"])["profitable"]
            .mean()
            .round()
            .astype(int)
            .to_dict()
        )

        # Predict each test sample using its configuration
        y_pred = [
            config_majority[(vf, if_)]
            for vf, if_ in zip(test["vf"], test["if"])
        ]

        y_true = test["profitable"].astype(int).tolist()

        all_y_true.extend(y_true)
        all_y_pred.extend(y_pred)

        accuracy = accuracy_score(y_true, y_pred)

        print(
            f"Group {fold:2d}: "
            f"{test['benchmark'].iloc[0]}::"
            f"{test['loop_id'].iloc[0]:<20} "
            f"accuracy={accuracy:.4f}"
        )

    print()
    print("-" * 70)
    print("LOGO OVERALL")
    print("-" * 70)

    accuracy = accuracy_score(all_y_true, all_y_pred)
    balanced = balanced_accuracy_score(all_y_true, all_y_pred)

    print(f"Accuracy           : {accuracy:.4f}")
    print(f"Balanced accuracy  : {balanced:.4f}")

    print()
    print("Confusion matrix:")
    print(confusion_matrix(all_y_true, all_y_pred))

    print()
    print("Classification report:")
    print(
        classification_report(
            all_y_true,
            all_y_pred,
            target_names=["unprofitable", "profitable"],
            digits=4,
        )
    )


if __name__ == "__main__":
    main()
