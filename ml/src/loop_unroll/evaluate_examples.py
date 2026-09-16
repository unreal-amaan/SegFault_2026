import joblib
import pandas as pd

DATASET = "data/processed/loop_unroll/loop_unroll.csv"
MODEL = "ml/models/loop_unroll_random_forest.joblib"

artifact = joblib.load(MODEL)
model = artifact["model"]
FEATURES = artifact["features"]

df = pd.read_csv(DATASET)

# Select loops that have all four candidates: UF 2, 4, 8, 16.
group_cols = ["benchmark", "function", "loop_id"]

complete_loops = (
    df.groupby(group_cols)["unroll_factor"]
    .apply(lambda x: set(x) == {2, 4, 8, 16})
)

complete_loops = complete_loops[complete_loops].index

# Take the first 10 complete loops.
selected_loops = list(complete_loops[:10])

print(f"Testing {len(selected_loops)} loops\n")

for benchmark, function, loop_id in selected_loops:
    group = df[
        (df["benchmark"] == benchmark)
        & (df["function"] == function)
        & (df["loop_id"] == loop_id)
    ].sort_values("unroll_factor")

    X = group[FEATURES]

    predictions = model.predict(X)
    probabilities = model.predict_proba(X)[:, 1]

    print("=" * 70)
    print(f"Benchmark : {benchmark}")
    print(f"Function  : {function}")
    print(f"Loop ID   : {loop_id}")
    print()
    print("UF    Actual    Predicted    P(profitable)")
    print("-" * 45)

    for uf, actual, predicted, probability in zip(
        group["unroll_factor"],
        group["profitable"],
        predictions,
        probabilities,
    ):
        print(
            f"{uf:<5} "
            f"{actual:<9} "
            f"{predicted:<12} "
            f"{probability:.3f}"
        )

print("=" * 70)