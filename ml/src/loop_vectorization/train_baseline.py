#!/usr/bin/env python3

import json
from pathlib import Path

import joblib

import numpy as np
import pandas as pd

from sklearn.ensemble import HistGradientBoostingClassifier, RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import (
    accuracy_score,
    balanced_accuracy_score,
    classification_report,
    confusion_matrix,
    roc_auc_score,
)
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import GroupKFold, LeaveOneGroupOut


ROOT = Path(__file__).resolve().parents[3]

DATASET = (
    ROOT
    / "data"
    / "processed"
    / "loop_vectorization"
    / "vectorization_dataset.csv"
)


# Features that actually vary in the current dataset.
#
# benchmark and loop_id are metadata only.
# They must NOT be used as ML features because they would allow
# the model to memorize benchmark/loop-specific behavior.
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


def evaluate_model(name, model, X, y, groups):
    print()
    print("=" * 70)
    print(name)
    print("=" * 70)

    cv = GroupKFold(n_splits=5)

    all_y_true = []
    all_y_pred = []
    all_y_prob = []

    fold_results = []

    for fold, (train_idx, test_idx) in enumerate(
        cv.split(X, y, groups), start=1
    ):
        X_train = X.iloc[train_idx]
        X_test = X.iloc[test_idx]

        y_train = y.iloc[train_idx]
        y_test = y.iloc[test_idx]

        model.fit(X_train, y_train)

        y_pred = model.predict(X_test)

        if hasattr(model, "predict_proba"):
            y_prob = model.predict_proba(X_test)[:, 1]
        else:
            # HistGradientBoosting also supports predict_proba,
            # but keep this fallback for future models.
            y_prob = model.decision_function(X_test)

        accuracy = accuracy_score(y_test, y_pred)
        balanced = balanced_accuracy_score(y_test, y_pred)

        fold_results.append((accuracy, balanced))

        all_y_true.extend(y_test)
        all_y_pred.extend(y_pred)
        all_y_prob.extend(y_prob)

        print(
            f"Fold {fold}: "
            f"train={len(train_idx):3d} "
            f"test={len(test_idx):3d} "
            f"accuracy={accuracy:.4f} "
            f"balanced_accuracy={balanced:.4f}"
        )

    all_y_true = np.asarray(all_y_true)
    all_y_pred = np.asarray(all_y_pred)
    all_y_prob = np.asarray(all_y_prob)

    accuracy = accuracy_score(all_y_true, all_y_pred)
    balanced = balanced_accuracy_score(all_y_true, all_y_pred)
    auc = roc_auc_score(all_y_true, all_y_prob)

    print()
    print("-" * 70)
    print("OVERALL")
    print("-" * 70)
    print(f"Accuracy           : {accuracy:.4f}")
    print(f"Balanced accuracy  : {balanced:.4f}")
    print(f"ROC-AUC            : {auc:.4f}")

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

    return {
        "model": name,
        "accuracy": accuracy,
        "balanced_accuracy": balanced,
        "roc_auc": auc,
    }

def evaluate_logo(name, model, X, y, groups):
    print()
    print("=" * 70)
    print(f"{name} - LEAVE-ONE-GROUP-OUT")
    print("=" * 70)

    cv = LeaveOneGroupOut()

    all_y_true = []
    all_y_pred = []
    all_y_prob = []

    fold_results = []

    for fold, (train_idx, test_idx) in enumerate(
        cv.split(X, y, groups), start=1
    ):
        X_train = X.iloc[train_idx]
        X_test = X.iloc[test_idx]

        y_train = y.iloc[train_idx]
        y_test = y.iloc[test_idx]

        model.fit(X_train, y_train)

        y_pred = model.predict(X_test)
        y_prob = model.predict_proba(X_test)[:, 1]

        accuracy = accuracy_score(y_test, y_pred)
        balanced = balanced_accuracy_score(y_test, y_pred)

        fold_results.append((accuracy, balanced))

        all_y_true.extend(y_test)
        all_y_pred.extend(y_pred)
        all_y_prob.extend(y_prob)

        group_name = groups.iloc[test_idx].iloc[0]

        print(
            f"Group {fold:2d}: "
            f"{group_name:25s} "
            f"test={len(test_idx):2d} "
            f"accuracy={accuracy:.4f} "
            f"balanced_accuracy={balanced:.4f}"
        )

    all_y_true = np.asarray(all_y_true)
    all_y_pred = np.asarray(all_y_pred)
    all_y_prob = np.asarray(all_y_prob)

    accuracy = accuracy_score(all_y_true, all_y_pred)
    balanced = balanced_accuracy_score(all_y_true, all_y_pred)
    auc = roc_auc_score(all_y_true, all_y_prob)

    print()
    print("-" * 70)
    print("LOGO OVERALL")
    print("-" * 70)
    print(f"Accuracy           : {accuracy:.4f}")
    print(f"Balanced accuracy  : {balanced:.4f}")
    print(f"ROC-AUC            : {auc:.4f}")

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

    return {
        "model": name,
        "accuracy": accuracy,
        "balanced_accuracy": balanced,
        "roc_auc": auc,
    }

def main():
    if not DATASET.exists():
        raise FileNotFoundError(f"Dataset not found: {DATASET}")

    df = pd.read_csv(DATASET)

    print("=" * 70)
    print("VECTORZATION COST MODEL EXPERIMENT")
    print("=" * 70)

    print(f"Dataset : {DATASET}")
    print(f"Samples : {len(df)}")
    print(f"Groups  : {df.groupby(['benchmark', 'loop_id']).ngroups}")
    print(f"Features: {len(FEATURES)}")

    X = df[FEATURES]
    y = df["profitable"].astype(int)

    # Each benchmark + loop combination is one group.
    #
    # This is critical because each loop has multiple VF/IF
    # configurations. We must never train on configurations
    # from a loop and test on other configurations of that
    # same loop.
    groups = (
        df["benchmark"].astype(str)
        + "::"
        + df["loop_id"].astype(str)
    )

    print()
    print("Target distribution:")
    print(y.value_counts().sort_index())

    print()
    print("Feature count:")
    print(f"Using {len(FEATURES)} varying features")
    print("benchmark and loop_id are metadata only")

    models = {
        "Logistic Regression": Pipeline(
            [
                ("scaler", StandardScaler()),
                (
                    "model",
                    LogisticRegression(
                        max_iter=2000,
                        random_state=42,
                    ),
                ),
            ]
        ),

        "Random Forest": RandomForestClassifier(
            n_estimators=300,
            max_depth=None,
            min_samples_leaf=2,
            random_state=42,
            n_jobs=-1,
        ),

        "HistGradientBoosting": HistGradientBoostingClassifier(
            max_iter=200,
            learning_rate=0.05,
            max_leaf_nodes=15,
            l2_regularization=1.0,
            random_state=42,
        ),
    }

    results = []

    for name, model in models.items():
        results.append(
            evaluate_model(
                name,
                model,
                X,
                y,
                groups,
            )
        )

    print()
    print("=" * 70)
    print("LEAVE-ONE-GROUP-OUT RESULTS")
    print("=" * 70)

    logo_results = []

    for name, model in models.items():
        logo_results.append(
            evaluate_logo(
                name,
                model,
                X,
                y,
                groups,
            )
        )

    print()
    print("=" * 70)
    print("MODEL COMPARISON")
    print("=" * 70)

    results_df = pd.DataFrame(results)
    results_df = results_df.sort_values(
        "balanced_accuracy",
        ascending=False,
    )

    print(
        results_df.to_string(
            index=False,
            float_format=lambda x: f"{x:.4f}",
        )
    )

    print()
    print("=" * 70)
    print("REFERENCE BASELINE")
    print("=" * 70)

    majority = y.mode()[0]
    majority_accuracy = (y == majority).mean()

    print(f"Majority label     : {majority}")
    print(f"Majority accuracy  : {majority_accuracy:.4f}")

    print()
    print("Note:")
    print("The reference baseline uses the complete dataset distribution.")
    print("The model results above use 5-fold GroupKFold evaluation.")
    print()
    print("=" * 70)
    print("FINAL MODEL TRAINING")
    print("=" * 70)

    final_model = RandomForestClassifier(
        n_estimators=300,
        max_depth=None,
        min_samples_leaf=2,
        random_state=42,
        n_jobs=-1,
    )

    final_model.fit(X, y)

    model_dir = ROOT / "ml" / "models"
    model_dir.mkdir(parents=True, exist_ok=True)

    model_path = model_dir / "loop_vectorization_random_forest.joblib"

    joblib.dump(
        {
            "model": final_model,
            "features": FEATURES,
        },
        model_path,
    )

    print(f"Final model trained on {len(X)} samples")
    print(f"Saved to: {model_path}")


if __name__ == "__main__":
    main()
