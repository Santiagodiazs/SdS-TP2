"""
cellular_automata_sweep_plot.py

Lee resources/sweep_summary.csv (generado por run_cellular_automata_sweep.py)
y produce las figuras finales pedidas por el TP:
  c) va vs eta, con barras de error, una curva por densidad.
  d) S vs eta, con barras de error, una curva por densidad.
  e) va vs S (scatter), distinguiendo densidades.
  f) Las anteriores, comparando el modelo estandar (vicsek) vs votante.

Uso:
    python cellular_automata_sweep_plot.py resources/sweep_summary.csv
"""

import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

MARKERS = ["o", "s", "^", "D", "v"]
VA_VS_S_ERROR_ALPHA = 0.28


def load_summary(path: Path) -> pd.DataFrame:
    df = pd.read_csv(path)
    return df


def aggregate_over_seeds(df: pd.DataFrame) -> pd.DataFrame:
    """
    El csv tiene una fila por (model, density, noise, seed). Para graficar
    con barras de error 'entre realizaciones' hay que:
      - promediar los va_mean/s_mean de cada seed (da el punto central)
      - usar el desvio ENTRE seeds (no el va_std/s_std intra-corrida) como
        barra de error, porque lo que el TP pide es variabilidad entre
        realizaciones distintas, no la fluctuacion temporal dentro de una
        corrida (esa ya se descarto al promediar en el estacionario).
    """
    grouped = df.groupby(["model", "density", "noise"]).agg(
        va_avg=("va_mean", "mean"),
        va_err=("va_mean", "std"),
        s_avg=("s_mean", "mean"),
        s_err=("s_mean", "std"),
    ).reset_index()
    return grouped


def plot_observable_vs_noise(df: pd.DataFrame, model: str, value_col: str, err_col: str,
                              ylabel: str, title: str, output_path=None):
    fig, ax = plt.subplots(figsize=(7, 5))
    subset = df[df["model"] == model]

    for i, density in enumerate(sorted(subset["density"].unique())):
        density_data = subset[subset["density"] == density].sort_values("noise")
        ax.errorbar(density_data["noise"], density_data[value_col], yerr=density_data[err_col],
                    marker=MARKERS[i % len(MARKERS)], label=f"rho={density}", capsize=3)

    ax.set_xlabel("Ruido (eta)")
    ax.set_ylabel(ylabel)
    ax.set_title(f"{title} - modelo {model}")
    ax.legend()
    fig.tight_layout()

    if output_path:
        fig.savefig(output_path, dpi=150)
        print(f"Guardado: {output_path}")
    else:
        plt.show()
    plt.close(fig)


def plot_va_vs_s(df: pd.DataFrame, model: str, output_path=None):
    fig, ax = plt.subplots(figsize=(7, 5))
    subset = df[df["model"] == model]

    for i, density in enumerate(sorted(subset["density"].unique())):
        # Unir los puntos en orden creciente de eta muestra la trayectoria
        # del sistema en el plano (S, v_a) para cada densidad.
        density_data = subset[subset["density"] == density].sort_values("noise")
        line, = ax.plot(density_data["s_avg"], density_data["va_avg"],
                        marker=MARKERS[i % len(MARKERS)], linestyle="-",
                        label=f"rho={density}", zorder=2)
        # Las barras se atenúan para que indiquen la dispersión sin ocultar
        # la trayectoria central ni los marcadores.
        ax.errorbar(density_data["s_avg"], density_data["va_avg"],
                    xerr=density_data["s_err"], yerr=density_data["va_err"],
                    fmt="none", ecolor=line.get_color(), alpha=VA_VS_S_ERROR_ALPHA,
                    elinewidth=1, capsize=3, zorder=1)

    ax.set_xlabel("Fraccion del cluster mas grande (S)")
    ax.set_ylabel("Parametro de orden (v_a)")
    ax.set_title(f"v_a vs S - modelo {model}")
    ax.legend()
    fig.tight_layout()

    if output_path:
        fig.savefig(output_path, dpi=150)
        print(f"Guardado: {output_path}")
    else:
        plt.show()
    plt.close(fig)


def plot_model_comparison(df: pd.DataFrame, density: float, value_col: str, err_col: str,
                           ylabel: str, title: str, output_path=None):
    """Punto (f): compara vicsek vs voter para una densidad fija."""
    fig, ax = plt.subplots(figsize=(7, 5))

    for i, model in enumerate(sorted(df["model"].unique())):
        model_data = df[(df["model"] == model) & (df["density"] == density)].sort_values("noise")
        ax.errorbar(model_data["noise"], model_data[value_col], yerr=model_data[err_col],
                    marker=MARKERS[i % len(MARKERS)], label=model, capsize=3)

    ax.set_xlabel("Ruido (eta)")
    ax.set_ylabel(ylabel)
    ax.set_title(f"{title} - rho={density} - comparacion de modelos")
    ax.legend()
    fig.tight_layout()

    if output_path:
        fig.savefig(output_path, dpi=150)
        print(f"Guardado: {output_path}")
    else:
        plt.show()
    plt.close(fig)


def plot_va_vs_s_model_comparison(df: pd.DataFrame, output_path=None):
    """Punto (f): compara ambos modelos en el plano (S, v_a)."""
    fig, ax = plt.subplots(figsize=(8, 6))
    line_styles = {"vicsek": "-", "voter": "--"}
    colors = {density: f"C{i}" for i, density in enumerate(sorted(df["density"].unique()))}

    for density in sorted(df["density"].unique()):
        for model in sorted(df["model"].unique()):
            density_data = df[(df["density"] == density) & (df["model"] == model)].sort_values("noise")
            line, = ax.plot(density_data["s_avg"], density_data["va_avg"],
                            color=colors[density], linestyle=line_styles[model], marker="o",
                            label=f"{model}, rho={density}", zorder=2)
            ax.errorbar(density_data["s_avg"], density_data["va_avg"],
                        xerr=density_data["s_err"], yerr=density_data["va_err"],
                        fmt="none", ecolor=line.get_color(), alpha=VA_VS_S_ERROR_ALPHA,
                        elinewidth=1, capsize=3, zorder=1)

    ax.set_xlabel("Fraccion del cluster mas grande (S)")
    ax.set_ylabel("Parametro de orden (v_a)")
    ax.set_title("v_a vs S - comparacion Vicsek vs votante")
    ax.legend(ncol=2, fontsize="small")
    fig.tight_layout()
    if output_path:
        fig.savefig(output_path, dpi=150)
        print(f"Guardado: {output_path}")
    else:
        plt.show()
    plt.close(fig)


def main():
    if len(sys.argv) < 2:
        print("Uso: python cellular_automata_sweep_plot.py <ruta_a_sweep_summary.csv> [directorio_salida]")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    if output_dir:
        output_dir.mkdir(parents=True, exist_ok=True)

    raw_df = load_summary(input_path)
    df = aggregate_over_seeds(raw_df)

    def out(name):
        return str(output_dir / name) if output_dir else None

    for model in df["model"].unique():
        plot_observable_vs_noise(df, model, "va_avg", "va_err",
                                  ylabel="Parametro de orden (v_a)",
                                  title="v_a vs eta",
                                  output_path=out(f"va_vs_eta_{model}.png"))
        plot_observable_vs_noise(df, model, "s_avg", "s_err",
                                  ylabel="Fraccion del cluster mas grande (S)",
                                  title="S vs eta",
                                  output_path=out(f"s_vs_eta_{model}.png"))
        plot_va_vs_s(df, model, output_path=out(f"va_vs_s_{model}.png"))

    for density in df["density"].unique():
        plot_model_comparison(df, density, "va_avg", "va_err",
                               ylabel="Parametro de orden (v_a)",
                               title="v_a vs eta",
                               output_path=out(f"va_vs_eta_comparison_rho{density}.png"))
        plot_model_comparison(df, density, "s_avg", "s_err",
                               ylabel="Fraccion del cluster mas grande (S)",
                               title="S vs eta",
                               output_path=out(f"s_vs_eta_comparison_rho{density}.png"))

    plot_va_vs_s_model_comparison(df, output_path=out("va_vs_s_model_comparison.png"))


if __name__ == "__main__":
    main()
