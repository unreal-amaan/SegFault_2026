import joblib
import pandas as pd
from sklearn.model_selection import GroupKFold
from sklearn.metrics import accuracy_score, f1_score

DATASET = "data/processed/loop_unroll/loop_unroll.csv"
MODEL = "ml/models/loop_unroll_random_forest.joblib"

GROUP_COLS = ["benchmark", "function", "loop_id"]

df = pd.read_csv(DATASET)

artifact = joblib.load(MODEL)
features = artifact["features"]
model_template = artifact["model"]

# Treat each source loop as one group.
groups = (
    df["benchmark"].astype(str)
    + "/"
    + df["function"].astype(str)
    + "/"
    + df["loop_id"].astype(str)
)

gkf = GroupKFold(n_splits=5)

all_predictions = []
all_actuals = []

print("Unseen-loop evaluation")
print("=" * 70)

for fold, (train_idx, test_idx) in enumerate(
    gkf.split(df, df["profitable"], groups=groups),
    start=1,
):
    train = df.iloc[train_idx]
    test = df.iloc[test_idx]

    model = model_template.__class__(
        n_estimators=200,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
    )

    model.fit(train[features], train["profitable"])

    predictions = model.predict(test[features])

    all_predictions.extend(predictions)
    all_actuals.extend(test["profitable"])

    accuracy = accuracy_score(test["profitable"], predictions)
    f1 = f1_score(test["profitable"], predictions)

    print(f"Fold {fold}: accuracy={accuracy:.3f}, F1={f1:.3f}")

print("=" * 70)

overall_accuracy = accuracy_score(all_actuals, all_predictions)
overall_f1 = f1_score(all_actuals, all_predictions)

print(f"Overall candidate accuracy: {overall_accuracy:.3f}")
print(f"Overall candidate F1:       {overall_f1:.3f}")

# ------------------------------------------------------------
# Per-loop evaluation
# ------------------------------------------------------------

results = []

for fold, (train_idx, test_idx) in enumerate(
    gkf.split(df, df["profitable"], groups=groups),
    start=1,
):
    train = df.iloc[train_idx]
    test = df.iloc[test_idx]

    model = model_template.__class__(
        n_estimators=200,
        random_state=42,
        class_weight="balanced",
        n_jobs=-1,
    )

    model.fit(train[features], train["profitable"])

    test = test.copy()
    test["prediction"] = model.predict(test[features])

    for loop_key, loop in test.groupby(GROUP_COLS):
        correct = (loop["prediction"] == loop["profitable"]).sum()

        results.append(
            {
                "benchmark": loop_key[0],
                "function": loop_key[1],
                "loop_id": loop_key[2],
                "correct": correct,
                "candidates": len(loop),
                "accuracy": correct / len(loop),
            }
        )

results_df = pd.DataFrame(results)

print()
print("Per-loop results")
print("=" * 70)

for _, row in results_df.iterrows():
    print(
        f"{row['benchmark']}/{row['function']}/loop-{row['loop_id']}: "
        f"{int(row['correct'])}/{int(row['candidates'])}"
    )