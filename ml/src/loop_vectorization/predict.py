#!/usr/bin/env python3

from pathlib import Path

import joblib
import pandas as pd


ROOT = Path(__file__).resolve().parents[3]

MODEL_PATH = (
    ROOT
    / "ml"
    / "models"
    / "loop_vectorization_random_forest.joblib"
)

CONFIGURATIONS = [
    (2, 1),
    (2, 2),
    (2, 4),
    (4, 1),
    (4, 2),
    (4, 4),
    (8, 1),
    (8, 2),
    (8, 4),
    (16, 1),
    (16, 2),
    (16, 4),
]


def predict_configurations(features):
    bundle = joblib.load(MODEL_PATH)

    model = bundle["model"]
    feature_names = bundle["features"]

    base_features = {
        key: value
        for key, value in features.items()
        if key not in ("vf", "if")
    }

    rows = []

    for vf, interleave_factor in CONFIGURATIONS:
        row = dict(base_features)
        row["vf"] = vf
        row["if"] = interleave_factor
        rows.append(row)

    X = pd.DataFrame(rows)[feature_names]

    probabilities = model.predict_proba(X)[:, 1]
    predictions = model.predict(X)

    results = []

    for (vf, interleave_factor), prediction, probability in zip(
        CONFIGURATIONS,
        predictions,
        probabilities,
    ):
        results.append(
            {
                "vf": vf,
                "if": interleave_factor,
                "profitable": int(prediction),
                "probability": float(probability),
            }
        )

    results.sort(
        key=lambda x: x["probability"],
        reverse=True,
    )

    return results


def main():
    # Example loop features.
    #
    # These should eventually come directly from
    # the LLVM LoopFeatureExtractor.
    features = {
        "loop_depth": 3,
        "num_instructions": 11,
        "num_phi_nodes": 2,
        "num_loads": 2,
        "num_stores": 1,
        "num_forward_contiguous_loads": 1,
        "num_strided_loads": 0,
        "num_forward_contiguous_stores": 1,
        "num_float_ops": 1,
        "has_reduction": 0,
        "num_reductions": 0,
        "num_calls": 0,
        "trip_count": 900,
        "memory_op_ratio": 0.272727,
        "control_overhead_ratio": 0.090909,
        "arithmetic_intensity": 0.333333,
    }

    results = predict_configurations(features)

    print("=" * 70)
    print("VECTOR CONFIGURATION PREDICTION")
    print("=" * 70)

    print()
    print(f"{'Rank':<6}{'VF':<6}{'IF':<6}{'Probability':<15}{'Prediction'}")
    print("-" * 70)

    for rank, result in enumerate(results, start=1):
        label = (
            "PROFITABLE"
            if result["profitable"]
            else "UNPROFITABLE"
        )

        print(
            f"{rank:<6}"
            f"{result['vf']:<6}"
            f"{result['if']:<6}"
            f"{result['probability']:<15.4f}"
            f"{label}"
        )

    best = results[0]

    print()
    print("=" * 70)
    print("RECOMMENDED CONFIGURATION")
    print("=" * 70)

    print(
        f"VF = {best['vf']}, "
        f"IF = {best['if']}"
    )

    print(
        f"Profitability probability = "
        f"{best['probability']:.4f}"
    )


if __name__ == "__main__":
    main()