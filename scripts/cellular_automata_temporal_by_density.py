"""Genera v_a(t) y S(t) con todas las densidades en una misma figura.

Uso:
  python cellular_automata_temporal_by_density.py <prefijo_salida> \
      --series <rho> <observables.txt> [--series <rho> <observables.txt> ...]
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from cellular_automata_observables import (
    detect_steady_state_start,
    parse_observables,
)


def plot_all_densities(series, observable_index, ylabel, title, output_path):
    fig, ax = plt.subplots(figsize=(8, 5))

    for density, t_vals, va_vals, s_vals in sorted(series, key=lambda item: item[0]):
        values = va_vals if observable_index == 0 else s_vals
        steady_start = detect_steady_state_start(values)
        line, = ax.plot(t_vals, values, linewidth=1.2, label=fr"$\rho={density:g}$")
        ax.axvline(t_vals[steady_start], color=line.get_color(), linestyle="--", alpha=0.75)

    ax.set_xlabel("Paso temporal (t)")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend(title="Densidad")
    ax.grid(alpha=0.2)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"Guardado: {output_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_prefix", type=Path)
    parser.add_argument("--series", nargs=2, action="append", metavar=("RHO", "ARCHIVO"),
                        required=True, help="Densidad y su archivo de observables")
    args = parser.parse_args()

    series = []
    for density_text, path_text in args.series:
        t_vals, va_vals, s_vals = parse_observables(Path(path_text))
        series.append((float(density_text), t_vals, va_vals, s_vals))

    args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
    plot_all_densities(series, 0, "Parametro de orden ($v_a$)",
                       "Evolucion temporal de $v_a$", f"{args.output_prefix}_va_evolution.png")
    plot_all_densities(series, 1, "Fraccion del cluster mas grande (S)",
                       "Evolucion temporal de S", f"{args.output_prefix}_s_evolution.png")


if __name__ == "__main__":
    main()
