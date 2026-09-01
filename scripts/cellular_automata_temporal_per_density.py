"""Genera una figura temporal por densidad: v_a(t) y S(t), Vicsek vs votante.

Uso:
  python cellular_automata_temporal_per_density.py <prefijo_salida> \
      --series <rho> <modelo> <observables.txt> [--series ...]
"""

import argparse
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

from cellular_automata_observables import detect_steady_state_start, parse_observables


MODEL_STYLES = {"vicsek": ("tab:blue", "-"), "voter": ("tab:orange", "--")}


def plot_density(density, model_series, output_path):
    fig, (va_ax, s_ax) = plt.subplots(2, 1, figsize=(8, 7), sharex=True)

    for model, t_vals, va_vals, s_vals in sorted(model_series):
        color, linestyle = MODEL_STYLES[model]
        va_start = detect_steady_state_start(va_vals)
        s_start = detect_steady_state_start(s_vals)

        va_ax.plot(t_vals, va_vals, color=color, linestyle=linestyle, linewidth=1.2,
                   label=model.capitalize())
        va_ax.axvline(t_vals[va_start], color=color, linestyle=linestyle, alpha=0.35)

        s_ax.plot(t_vals, s_vals, color=color, linestyle=linestyle, linewidth=1.2,
                  label=model.capitalize())
        s_ax.axvline(t_vals[s_start], color=color, linestyle=linestyle, alpha=0.35)

    va_ax.set_ylabel("Parametro de orden ($v_a$)")
    va_ax.set_title(fr"Evolucion temporal: $\rho={density:g}$")
    va_ax.legend()
    va_ax.grid(alpha=0.2)

    s_ax.set_xlabel("Paso temporal (t)")
    s_ax.set_ylabel("Fraccion del cluster mas grande (S)")
    s_ax.grid(alpha=0.2)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"Guardado: {output_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_prefix", type=Path)
    parser.add_argument("--series", nargs=3, action="append", metavar=("RHO", "MODELO", "ARCHIVO"),
                        required=True)
    args = parser.parse_args()

    by_density = defaultdict(list)
    for density_text, model, path_text in args.series:
        if model not in MODEL_STYLES:
            parser.error(f"Modelo no soportado: {model}")
        t_vals, va_vals, s_vals = parse_observables(Path(path_text))
        by_density[float(density_text)].append((model, t_vals, va_vals, s_vals))

    args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
    for density, model_series in by_density.items():
        found_models = {model for model, *_ in model_series}
        if found_models != set(MODEL_STYLES):
            parser.error(f"Faltan series para rho={density}: se requieren Vicsek y votante")
        plot_density(density, model_series, f"{args.output_prefix}_rho{density:g}.png")


if __name__ == "__main__":
    main()
