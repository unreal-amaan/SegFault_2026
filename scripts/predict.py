#!/usr/bin/env python3

import argparse
import json
import re
import subprocess
from pathlib import Path

import joblib
import pandas as pd


# ============================================================
# Paths
# ============================================================

ROOT = Path(__file__).resolve().parents[1]

LLVM_ROOT = ROOT.parent.parent / "llvm"

CLANG = LLVM_ROOT / "build" / "bin" / "clang"
OPT = LLVM_ROOT / "build" / "bin" / "opt"

PASS = ROOT / "build" / "llvm" / "passes" / "CompilerCostModel.so"
HEURISTIC_PASS = (
    ROOT / "build" / "llvm" / "passes" / "LLVMHeuristicCostModel.so"
)

POLYBENCH_ROOT = (
    ROOT.parent
    / "compiler-cost-model-data"
    / "benchmarks"
    / "polybench"
)

POLYBENCH_INCLUDE = POLYBENCH_ROOT / "utilities"

RESULTS_DIR = ROOT / "data" / "results"

MODELS = {
    "unroll": ROOT / "ml" / "models" / "loop_unroll_random_forest.joblib",
    "vectorize": ROOT / "ml" / "models" / "loop_vectorization_random_forest.joblib",
    "tiling": ROOT / "ml" / "models" / "loop_tiling_random_forest.joblib",
    "fusion": ROOT / "ml" / "models" / "loop_fusion_random_forest.joblib",
}


# ============================================================
# Compile source → LLVM IR
# ============================================================

def compile_to_ir(source: Path) -> Path:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    output_ir = RESULTS_DIR / f"{source.stem}.ll"

    print("\n[+] Compiling source to LLVM IR...")

    command = [
        str(CLANG),
        "-S",
        "-emit-llvm",
        "-O0",
        "-Xclang",
        "-disable-O0-optnone",
        "-I",
        str(POLYBENCH_INCLUDE),
        str(source),
        "-o",
        str(output_ir),
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print("\n[!] Clang failed:")
        print(result.stderr)
        raise RuntimeError("Failed to compile source to LLVM IR.")

    print(f"[+] LLVM IR written to:\n    {output_ir}")

    return output_ir


# ============================================================
# LLVM IR → feature JSON
# ============================================================

def extract_features(ir_file: Path) -> Path:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    output_json = RESULTS_DIR / f"{ir_file.stem}.features.json"

    pass_output = ROOT / "costmodel_features.json"

    if pass_output.exists():
        pass_output.unlink()

    print("\n[+] Extracting compiler features...")

    command = [
        str(OPT),
        "-load-pass-plugin",
        str(PASS),
        "-passes=function(compiler-cost-model)",
        str(ir_file),
        "-disable-output",
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        cwd=ROOT,
    )

    if result.returncode != 0:
        print("\n[!] LLVM cost-model pass failed:")
        print(result.stderr)
        raise RuntimeError("Failed to extract compiler features.")

    if not pass_output.exists():
        raise RuntimeError("LLVM pass produced no JSON output.")

    pass_output.replace(output_json)

    print("[+] Feature extraction completed.")
    print(f"    JSON: {output_json}")

    return output_json


# ============================================================
# Load JSON
# ============================================================

def load_records(json_file: Path):
    with open(json_file) as f:
        return json.load(f)


# ============================================================
# Feature normalization
# ============================================================

def make_model_row(record):
    f = record["features"]
    p = record["parameters"]
    optimization = record["optimization"]

    row = dict(f)

    if "innermost" in row:
        row["is_innermost"] = row["innermost"]

    # --------------------------------------------------------
    # Unroll
    # --------------------------------------------------------

    if optimization == "unroll":
        row["unroll_factor"] = p.get("factor", 0)

    # --------------------------------------------------------
    # Vectorization
    # --------------------------------------------------------

    elif optimization == "vectorize":
        row["vf"] = p.get("vf", 0)
        row["if"] = p.get("if", 0)

        row["num_forward_contiguous_loads"] = row.get(
            "contiguous_loads", 0
        )

        row["num_strided_loads"] = row.get(
            "strided_loads", 0
        )

        row["num_forward_contiguous_stores"] = row.get(
            "contiguous_stores", 0
        )

    # --------------------------------------------------------
    # Tiling
    # --------------------------------------------------------

    elif optimization == "tiling":
        row["tile_size"] = p.get("tile_size", 0)

        row["num_forward_contiguous_loads"] = row.get(
            "contiguous_loads", 0
        )

        row["num_strided_loads"] = row.get(
            "strided_loads", 0
        )

        row["num_forward_contiguous_stores"] = row.get(
            "contiguous_stores", 0
        )

        row["num_strided_stores"] = row.get(
            "strided_stores", 0
        )

    return row


# ============================================================
# ML Prediction
# ============================================================

def predict_records(records):
    print("\n[+] Running ML predictions...")

    results = []

    loaded_models = {}

    for record in records:
        optimization = record["optimization"]

        if optimization not in MODELS:
            continue

        model_path = MODELS[optimization]

        if not model_path.exists():
            print(
                f"[!] Model not found for {optimization}: "
                f"{model_path}"
            )
            continue

        if optimization not in loaded_models:
            artifact = joblib.load(model_path)
            loaded_models[optimization] = artifact

        artifact = loaded_models[optimization]

        model = artifact["model"]
        features = artifact["features"]

        row = make_model_row(record)

        missing = [
            feature
            for feature in features
            if feature not in row
        ]

        if missing:
            print(
                f"[!] Skipping {optimization}: "
                f"missing features: {missing}"
            )
            continue

        X = pd.DataFrame(
            [[row[feature] for feature in features]],
            columns=features,
        )

        prediction = int(model.predict(X)[0])

        probability = None

        if hasattr(model, "predict_proba"):
            probability = float(
                model.predict_proba(X)[0][1]
            )

        result = {
            "optimization": optimization,
            "function": record["function"],
            "loop_name": record["loop_name"],
            "loop_id": record["loop_id"],
            "parameters": record["parameters"],
            "profitable": prediction,
        }

        if probability is not None:
            result["probability"] = probability

        results.append(result)

    return results


# ============================================================
# LLVM Heuristic Model
# ============================================================

def run_heuristic(ir_file: Path):
    print("\n[+] Running LLVM heuristic cost model...")

    if not HEURISTIC_PASS.exists():
        print(
            f"[!] Heuristic pass not found:\n"
            f"    {HEURISTIC_PASS}"
        )
        return []

    command = [
        str(OPT),
        "-load-pass-plugin",
        str(HEURISTIC_PASS),
        "-passes=function(llvm-heuristic-cost-model)",
        str(ir_file),
        "-disable-output",
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        cwd=ROOT,
    )

    if result.returncode != 0:
        print("\n[!] LLVM heuristic pass failed:")
        print(result.stderr)
        return []

    output = result.stderr

    heuristic_results = []

    current_function = None
    current_optimization = None
    current_result = None

    for line in output.splitlines():
        line = line.strip()

        if line.startswith("Function:"):
            current_function = line.split(":", 1)[1].strip()

        elif line.startswith("Optimization"):
            current_optimization = line.split(":", 1)[1].strip().lower()

        elif line.startswith("Loop "):
            if current_result:
                heuristic_results.append(current_result)

            loop_name = line.split(":", 1)[1].strip()

            current_result = {
                "optimization": current_optimization,
                "function": current_function,
                "loop": loop_name,
            }

        elif current_result is not None and line.startswith("instructions"):
            current_result["instructions"] = int(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("memory ops"):
            current_result["memory_ops"] = int(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("branches"):
            current_result["branches"] = int(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("LLVM cost"):
            current_result["llvm_cost"] = int(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("memory ratio"):
            current_result["memory_ratio"] = float(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("branch ratio"):
            current_result["branch_ratio"] = float(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("score"):
            current_result["score"] = float(
                line.split(":", 1)[1].strip()
            )

        elif current_result is not None and line.startswith("profitable"):
            value = line.split(":", 1)[1].strip()
            current_result["profitable"] = value == "YES"

    if current_result:
        heuristic_results.append(current_result)

    return heuristic_results

# ============================================================
# Display ML Results
# ============================================================

def print_ml_results(predictions):
    print("\n" + "=" * 90)
    print("ML PREDICTIONS")
    print("=" * 90)

    if not predictions:
        print("\n[!] No ML predictions produced.")
        return

    print(
        f"{'OPTIMIZATION':<15}"
        f"{'FUNCTION':<20}"
        f"{'LOOP':<20}"
        f"{'PARAMETERS':<25}"
        f"{'RESULT':<18}"
        f"{'PROBABILITY':>12}"
    )

    print("-" * 90)

    for result in predictions:
        status = (
            "PROFITABLE"
            if result["profitable"]
            else "NOT PROFITABLE"
        )

        probability = result.get("probability")

        probability_text = (
            f"{probability:.4f}"
            if probability is not None
            else "-"
        )

        print(
            f"{result['optimization'].upper():<15}"
            f"{result['function']:<20}"
            f"{result['loop_name']:<20}"
            f"{str(result['parameters']):<25}"
            f"{status:<18}"
            f"{probability_text:>12}"
        )


# ============================================================
# Display Heuristic Results
# ============================================================

def print_heuristic_results(results):
    print("\n" + "=" * 105)
    print("LLVM HEURISTIC PREDICTIONS")
    print("=" * 105)

    if not results:
        print("\n[!] No heuristic predictions produced.")
        return

    print(
        f"{'OPTIMIZATION':<15}"
        f"{'FUNCTION':<20}"
        f"{'LOOP':<18}"
        f"{'LLVM COST':<12}"
        f"{'MEM RATIO':<12}"
        f"{'BRANCH RATIO':<14}"
        f"{'SCORE':<10}"
        f"{'RESULT':<18}"
    )

    print("-" * 105)

    for result in results:
        status = (
            "PROFITABLE"
            if result.get("profitable", False)
            else "NOT PROFITABLE"
        )

        print(
            f"{result.get('optimization', '-').upper():<15}"
            f"{result.get('function', '-'):<20}"
            f"{result.get('loop', '-'):<18}"
            f"{result.get('llvm_cost', 0):<12}"
            f"{result.get('memory_ratio', 0.0):<12.4f}"
            f"{result.get('branch_ratio', 0.0):<14.4f}"
            f"{result.get('score', 0.0):<10.4f}"
            f"{status:<18}"
        )

# ============================================================
# Comparison
# ============================================================

def print_comparison(predictions, heuristic_results):
    print("\n" + "=" * 105)
    print("ML vs LLVM HEURISTIC COMPARISON")
    print("=" * 105)

    if not predictions:
        print("\n[!] No ML predictions available.")
        return

    if not heuristic_results:
        print("\n[!] No heuristic predictions available.")
        return

    heuristic_map = {}

    for result in heuristic_results:
        key = (
            result.get("optimization", "").lower(),
            result.get("function", ""),
            result.get("loop", ""),
        )
        heuristic_map[key] = result

    comparisons = []

    for ml in predictions:
        key = (
            ml["optimization"].lower(),
            ml["function"],
            ml["loop_name"],
        )

        heuristic = heuristic_map.get(key)

        if heuristic is None:
            continue

        comparisons.append(
            {
                "optimization": ml["optimization"],
                "function": ml["function"],
                "loop": ml["loop_name"],
                "ml_result": ml["profitable"],
                "ml_probability": ml.get("probability"),
                "heuristic_result": heuristic.get(
                    "profitable", False
                ),
                "heuristic_score": heuristic.get(
                    "score", 0.0
                ),
            }
        )

    if not comparisons:
        print("\n[!] No matching ML/heuristic candidates found.")
        return

    print(
        f"{'OPTIMIZATION':<15}"
        f"{'FUNCTION':<20}"
        f"{'LOOP':<18}"
        f"{'ML':<18}"
        f"{'HEURISTIC':<18}"
        f"{'AGREE':<10}"
    )

    print("-" * 105)

    for result in comparisons:
        ml_status = (
            "PROFITABLE"
            if result["ml_result"]
            else "NOT PROFITABLE"
        )

        heuristic_status = (
            "PROFITABLE"
            if result["heuristic_result"]
            else "NOT PROFITABLE"
        )

        agree = (
            "YES"
            if result["ml_result"] == result["heuristic_result"]
            else "NO"
        )

        print(
            f"{result['optimization'].upper():<15}"
            f"{result['function']:<20}"
            f"{result['loop']:<18}"
            f"{ml_status:<18}"
            f"{heuristic_status:<18}"
            f"{agree:<10}"
        )

    agreements = sum(
        1
        for result in comparisons
        if result["ml_result"] == result["heuristic_result"]
    )

    total = len(comparisons)

    print("\n" + "-" * 105)

    print(
        f"Agreement: {agreements}/{total} "
        f"({100.0 * agreements / total:.1f}%)"
    )

    print(
        "\nNOTE:"
        "\n  Both models are evaluated on the same annotated"
        "\n  optimization candidates."
        "\n  ML uses the trained optimization-specific model."
        "\n  LLVM heuristic uses LLVM target instruction costs"
        "\n  plus the hand-designed feature heuristic."
    )

# ============================================================
# Main
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="Compiler cost model inference pipeline."
    )

    parser.add_argument(
        "source",
        type=Path,
        help="C/C++ source file to analyze.",
    )

    parser.add_argument(
        "--heuristic",
        action="store_true",
        help="Also run the LLVM heuristic cost model and compare results.",
    )

    args = parser.parse_args()

    source = args.source.resolve()

    print("=" * 70)
    print("COMPILER COST MODEL")
    print("=" * 70)

    print("\n[+] Source:")
    print(f"    {source}")

    print("\n[+] LLVM:")
    print(f"    clang : {CLANG}")
    print(f"    opt   : {OPT}")
    print(f"    pass  : {PASS}")

    ir_file = compile_to_ir(source)

    json_file = extract_features(ir_file)

    records = load_records(json_file)

    predictions = predict_records(records)

    print_ml_results(predictions)

    heuristic_results = []

    if args.heuristic:
        print(f"\n[+] Heuristic pass:")
        print(f"    pass  : {HEURISTIC_PASS}")

        heuristic_results = run_heuristic(ir_file)

        print_heuristic_results(heuristic_results)

        print_comparison(
            predictions,
            heuristic_results,
        )

    print("\n[+] Done.")


if __name__ == "__main__":
    main()
