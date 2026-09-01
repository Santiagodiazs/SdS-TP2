"""Compara Vicsek y votante en v_a(t) y S(t), para todas las densidades.

Uso:
  python cellular_automata_temporal_model_comparison.py <prefijo_salida> \
      --series <modelo> <rho> <observables.txt> [--series ...]
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from cellular_automata_observables import detect_steady_state_start, parse_observables


MODEL_STYLES = {"vicsek": "-", "voter": "--"}


def plot_comparison(series, observable_index, ylabel, title, output_path):
    fig, ax = plt.subplots(figsize=(9, 5))
    colors = {density: f"C{index}" for index, density in
              enumerate(sorted({item[1] for item in series}))}

    for model, density, t_vals, va_vals, s_vals in sorted(series, key=lambda item: (item[1], item[0])):
        values = va_vals if observable_index == 0 else s_vals
        start = detect_steady_state_start(values)
        line, = ax.plot(t_vals, values, color=colors[density], linestyle=MODEL_STYLES[model],
                        linewidth=1.2, label=fr"{model.capitalize()}, $\rho={density:g}$")
        ax.axvline(t_vals[start], color=line.get_color(), linestyle=MODEL_STYLES[model], alpha=0.35)

    ax.set_xlabel("Paso temporal (t)")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend(ncol=2, fontsize="small")
    ax.grid(alpha=0.2)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"Guardado: {output_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_prefix", type=Path)
    parser.add_argument("--series", nargs=3, action="append", metavar=("MODELO", "RHO", "ARCHIVO"),
                        required=True)
    args = parser.parse_args()

    series = []
    for model, density_text, path_text in args.series:
        if model not in MODEL_STYLES:
            parser.error(f"Modelo no soportado: {model}")
        t_vals, va_vals, s_vals = parse_observables(Path(path_text))
        series.append((model, float(density_text), t_vals, va_vals, s_vals))

    args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
    plot_comparison(series, 0, "Parametro de orden ($v_a$)",
                    "Comparacion temporal de $v_a$: Vicsek vs votante",
                    f"{args.output_prefix}_va_evolution.png")
    plot_comparison(series, 1, "Fraccion del cluster mas grande (S)",
                    "Comparacion temporal de S: Vicsek vs votante",
                    f"{args.output_prefix}_s_evolution.png")


if __name__ == "__main__":
    main()
