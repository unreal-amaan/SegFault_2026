import json
import joblib
import pandas as pd

MODEL = "ml/models/loop_unroll_random_forest.joblib"
INPUT = "../compiler-cost-model-data/data/v2/raw/fdtd-2d_loop_unroll.json"

# Load trained model
artifact = joblib.load(MODEL)
model = artifact["model"]
FEATURES = artifact["features"]

# Load unseen benchmark
with open(INPUT) as f:
    data = json.load(f)

rows = []
records = []

for record in data["records"]:
    row = {
        feature: record["features"][feature]
        for feature in FEATURES
        if feature != "unroll_factor"
    }

    row["unroll_factor"] = record["unroll_factor"]

    rows.append(row)
    records.append(record)

X = pd.DataFrame(rows)[FEATURES]

# Model predictions
predictions = model.predict(X)
probabilities = model.predict_proba(X)[:, 1]

# Compare with actual labels
actual = [record["profitable"] for record in records]

correct = sum(pred == truth for pred, truth in zip(predictions, actual))
total = len(actual)

print("=" * 70)
print("UNSEEN FILE EVALUATION")
print("=" * 70)
print(f"Benchmark:  {data['benchmark']}")
print(f"Loops:      {data['num_loops']}")
print(f"Candidates: {data['num_candidates']}")
print()

for record, pred, prob, truth in zip(
    records, predictions, probabilities, actual
):
    status = "✓" if pred == truth else "✗"

    print(
        f"loop={record['loop_id']:2d} "
        f"UF={record['unroll_factor']:2d} "
        f"pred={'PROFITABLE' if pred else 'UNPROFITABLE':12s} "
        f"actual={'PROFITABLE' if truth else 'UNPROFITABLE':12s} "
        f"P={prob:.3f} "
        f"{status}"
    )

print()
print("=" * 70)
print(f"Correct:  {correct}/{total}")
print(f"Accuracy: {correct / total:.3f}")
print("=" * 70)