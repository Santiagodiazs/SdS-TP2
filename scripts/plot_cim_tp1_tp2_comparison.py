"""Grafica CIM TP1, CIM TP2 y fuerza bruta TP2 frente a N."""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def load_tp1(path: Path) -> pd.DataFrame:
    data = pd.read_csv(path)
    data = data[data["algorithm"] == "CELL_INDEX_METHOD"].copy()
    return data.groupby("N", as_index=False).agg(mean_ms=("mean_ms", "mean"), std_ms=("std_ms", "mean"))


def load_tp2(path: Path, algorithm: str) -> pd.DataFrame:
    data = pd.read_csv(path)
    # Compatibilidad con el primer CSV generado por el wrapper: sus filas ya
    # tienen el algoritmo, pero por haberse creado vacio no recibieron header.
    if "algorithm" not in data.columns:
        data = pd.read_csv(path, header=None,
                           names=["N", "length", "radius", "algorithm", "rep", "time_ns"])
    data = data[data["algorithm"] == algorithm].copy()
    data["time_ms"] = data["time_ns"] / 1_000_000.0
    return data.groupby("N", as_index=False).agg(mean_ms=("time_ms", "mean"), std_ms=("time_ms", "std"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tp1_csv", type=Path)
    parser.add_argument("tp2_csv", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    series = [
        ("CIM (TP1)", load_tp1(args.tp1_csv), "o", "tab:blue"),
        ("CIM (TP2)", load_tp2(args.tp2_csv, "CIM"), "s", "tab:green"),
        ("Fuerza bruta (TP2)", load_tp2(args.tp2_csv, "BRUTE_FORCE"), "^", "tab:red"),
    ]

    fig, ax = plt.subplots(figsize=(8, 5))
    for label, data, marker, color in series:
        ax.errorbar(data["N"], data["mean_ms"], yerr=data["std_ms"], marker=marker,
                    color=color, capsize=3, label=label)

    ax.set_xlabel("Numero de particulas (N)")
    ax.set_ylabel("Tiempo de busqueda promedio (ms)")
    ax.set_title("Comparacion de tiempos de busqueda")
    ax.legend()
    ax.grid(alpha=0.2)
    fig.tight_layout()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=150)
    plt.close(fig)
    print(f"Guardado: {args.output}")


if __name__ == "__main__":
    main()
