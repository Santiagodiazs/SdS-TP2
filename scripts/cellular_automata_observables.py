"""
cellular_automata_observables.py

Lee un archivo de observables generado por CellularAutomataSystem::run()
(formato: una linea por step, "t va s"), detecta a partir de que tiempo
el sistema entro en regimen estacionario, grafica la evolucion temporal
marcando ese punto con una linea vertical, y calcula el promedio/desvio
de va y S en esa ventana estacionaria.

Uso:
    python cellular_automata_observables.py resources/observables.txt
"""

import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


def parse_observables(path: Path):
    t_vals, va_vals, s_vals = [], [], []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            t, va, s = line.split()
            t_vals.append(int(t))
            va_vals.append(float(va))
            s_vals.append(float(s))
    return np.array(t_vals), np.array(va_vals), np.array(s_vals)


def detect_steady_state_start(values: np.ndarray, window: int = 50, rel_tol: float = 0.05) -> int:
    """
    Heuristica simple: recorre la serie con una ventana movil de tamaño
    'window' y busca el primer punto a partir del cual la variacion
    (desvio estandar relativo al promedio de esa ventana) se mantiene
    por debajo de 'rel_tol' hasta el final de la serie.

    Esto NO es el unico criterio valido -- es un punto de partida
    razonable. Si para tu sistema da resultados raros (por ejemplo,
    detecta el estacionario demasiado temprano o nunca lo detecta),
    ajusta 'window'/'rel_tol', o inspecciona el grafico a ojo y fija
    el valor a mano para las corridas que vas a mostrar en la
    presentacion/informe.
    """
    n = len(values)
    if n < window * 2:
        return n // 2  # serie muy corta, fallback: mitad de la serie

    for start in range(0, n - window):
        window_vals = values[start:start + window]
        mean = np.mean(window_vals)
        std = np.std(window_vals)
        rel_std = std / mean if abs(mean) > 1e-9 else std

        if rel_std < rel_tol:
            # Verificar que la ventana siguiente tambien sea estable
            # (evita falsos positivos por una meseta transitoria corta).
            remaining = values[start:]
            remaining_std = np.std(remaining)
            remaining_mean = np.mean(remaining)
            remaining_rel_std = remaining_std / remaining_mean if abs(remaining_mean) > 1e-9 else remaining_std
            if remaining_rel_std < rel_tol * 2:  # un poco mas laxo para toda la cola
                return start

    return n // 2  # no se encontro estabilizacion clara, fallback


def compute_steady_state_stats(values: np.ndarray, steady_start: int):
    window = values[steady_start:]
    return float(np.mean(window)), float(np.std(window))


def plot_observable_evolution(t_vals, values, steady_start, ylabel, title, output_path=None):
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(t_vals, values, linewidth=1)
    ax.axvline(t_vals[steady_start], color="red", linestyle="--",
               label=f"Inicio estacionario (t={t_vals[steady_start]})")
    ax.set_xlabel("Paso temporal (t)")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend()
    fig.tight_layout()

    if output_path:
        fig.savefig(output_path, dpi=150)
        print(f"Guardado: {output_path}")
    else:
        plt.show()

    plt.close(fig)


def main():
    if len(sys.argv) < 2:
        print("Uso: python cellular_automata_observables.py <ruta_a_observables.txt> [prefijo_salida]")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_prefix = sys.argv[2] if len(sys.argv) > 2 else None

    t_vals, va_vals, s_vals = parse_observables(input_path)

    va_steady_start = detect_steady_state_start(va_vals)
    s_steady_start = detect_steady_state_start(s_vals)

    va_mean, va_std = compute_steady_state_stats(va_vals, va_steady_start)
    s_mean, s_std = compute_steady_state_stats(s_vals, s_steady_start)

    print(f"v_a: estacionario desde t={t_vals[va_steady_start]}, "
          f"promedio={va_mean:.4f} +- {va_std:.4f}")
    print(f"S:   estacionario desde t={t_vals[s_steady_start]}, "
          f"promedio={s_mean:.4f} +- {s_std:.4f}")

    va_output = f"{output_prefix}_va_evolution.png" if output_prefix else None
    s_output = f"{output_prefix}_s_evolution.png" if output_prefix else None

    plot_observable_evolution(t_vals, va_vals, va_steady_start,
                               ylabel="Parametro de orden (v_a)",
                               title="Evolucion temporal de v_a",
                               output_path=va_output)
    plot_observable_evolution(t_vals, s_vals, s_steady_start,
                               ylabel="Fraccion del cluster mas grande (S)",
                               title="Evolucion temporal de S",
                               output_path=s_output)


if __name__ == "__main__":
    main()