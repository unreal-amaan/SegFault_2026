#!/usr/bin/env python3

import argparse
from pathlib import Path

import joblib
import pandas as pd


# ============================================================
# Paths
# ============================================================

ROOT = Path(__file__).resolve().parents[3]

MODEL_PATH = (
    ROOT
    / "ml"
    / "models"
    / "loop_fusion_random_forest.joblib"
)


# ============================================================
# Prediction
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="Predict loop-fusion profitability."
    )

    parser.add_argument(
        "input",
        type=Path,
        help="CSV file containing fusion candidates.",
    )

    args = parser.parse_args()

    # --------------------------------------------------------
    # Load model
    # --------------------------------------------------------

    print("[+] Loading model:")
    print(f"    {MODEL_PATH}")

    if not MODEL_PATH.exists():
        raise FileNotFoundError(
            f"Model not found:\n{MODEL_PATH}"
        )

    saved = joblib.load(MODEL_PATH)

    model = saved["model"]
    features = saved["features"]

    # --------------------------------------------------------
    # Load input
    # --------------------------------------------------------

    print("\n[+] Loading input:")
    print(f"    {args.input}")

    if not args.input.exists():
        raise FileNotFoundError(
            f"Input file not found:\n{args.input}"
        )

    df = pd.read_csv(args.input)

    # --------------------------------------------------------
    # Validate features
    # --------------------------------------------------------

    missing = [
        feature
        for feature in features
        if feature not in df.columns
    ]

    if missing:
        raise RuntimeError(
            "Input is missing required features:\n"
            + "\n".join(missing)
        )

    # --------------------------------------------------------
    # Predict
    # --------------------------------------------------------

    X = df[features]

    predictions = model.predict(X)
    probabilities = model.predict_proba(X)

    # Probability of class 1 = profitable
    profitable_probability = probabilities[:, 1]

    # --------------------------------------------------------
    # Display results
    # --------------------------------------------------------

    print("\n" + "=" * 70)
    print("LOOP FUSION PREDICTIONS")
    print("=" * 70)

    for index, prediction in enumerate(predictions):

        probability = profitable_probability[index]

        label = (
            "PROFITABLE"
            if prediction == 1
            else "UNPROFITABLE"
        )

        if "benchmark" in df.columns:
            benchmark = df.iloc[index]["benchmark"]
        else:
            benchmark = "unknown"

        loop1 = (
            df.iloc[index]["loop1_id"]
            if "loop1_id" in df.columns
            else "?"
        )

        loop2 = (
            df.iloc[index]["loop2_id"]
            if "loop2_id" in df.columns
            else "?"
        )

        print(
            f"\nCandidate {index + 1}"
        )

        print(
            f"  benchmark : {benchmark}"
        )

        print(
            f"  loops     : {loop1} + {loop2}"
        )

        print(
            f"  prediction: {label}"
        )

        print(
            f"  probability profitable: "
            f"{probability:.4f}"
        )

    # --------------------------------------------------------
    # Optional output dataframe
    # --------------------------------------------------------

    results = df.copy()

    results["prediction"] = predictions

    results["prediction_label"] = [
        "profitable" if value == 1 else "unprofitable"
        for value in predictions
    ]

    results["profitable_probability"] = (
        profitable_probability
    )

    output = (
        args.input.parent
        / f"{args.input.stem}_predictions.csv"
    )

    results.to_csv(
        output,
        index=False,
    )

    print("\n" + "=" * 70)
    print("OUTPUT")
    print("=" * 70)

    print(f"Saved predictions to:")
    print(f"    {output}")


if __name__ == "__main__":
    main()