"""
run_cellular_automata_sweep.py

Orquesta corridas del binario C++ (cellular_automata_app) barriendo:
  - modelo (vicsek / voter)
  - densidad (rho)
  - ruido (eta)
  - semillas (varias realizaciones para promediar)

Lee resources/observables.txt despues de cada corrida, calcula el promedio
y desvio de v_a y S en el regimen estacionario, y junta todo en un .csv
resumen que despues consume cellular_automata_sweep_plot.py.

OJO - LIMITACION IMPORTANTE:
El binario escribe siempre a resources/observables.txt y resources/frames.txt
(paths fijos, hardcodeados en CellularAutomataSystem::run()). Por eso este
script corre las simulaciones DE A UNA (serial), nunca en paralelo: si
lanzaras dos corridas al mismo tiempo, ambas pisarian el mismo archivo.
Si mas adelante agregas flags --frames-output / --observables-output al
main.cpp para que cada corrida escriba a un archivo distinto, recien ahi
tiene sentido paralelizar esto con concurrent.futures.
"""

import csv
import os
import subprocess
import sys
import math
from pathlib import Path

import numpy as np

EXECUTABLE = Path("build/apps/cellular_automata/cellular_automata_app")
# Permite que el wrapper de entrega aisle los archivos transitorios del sweep.
# Si no se define, conserva el comportamiento historico en resources/.
OUTPUT_DIRECTORY = Path(os.environ.get("TP2_SWEEP_OUTPUT", "resources"))
OBSERVABLES_PATH = OUTPUT_DIRECTORY / "observables.txt"

LENGTH = 10
INTERACTION_RADIUS = 1.0
STEPS = 2000
SEEDS_PER_POINT = 5
BURN_IN_FRACTION = 0.5  # se descarta el primer 50% de los steps como transitorio

DENSITIES = [1/math.pi, 1/(2*math.pi), 1/(3*math.pi), 2, 4, 8]
NOISES = np.linspace(0.0, 5.0, 11)
MODELS = ["vicsek", "voter"]


def run_simulation(model: str, density: float, noise: float, steps: int) -> None:
    """Corre una simulacion y deja el resultado en OBSERVABLES_PATH."""
    args = [
        str(EXECUTABLE),
        "--model", model,
        "--length", str(LENGTH),
        "--density", str(density),
        "--radius", str(INTERACTION_RADIUS),
        "--noise", str(noise),
        "--steps", str(steps),
        "--output", str(OUTPUT_DIRECTORY),
        "--no-frames",
    ]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"La simulacion fallo (model={model}, density={density}, noise={noise}):\n"
            f"stdout: {result.stdout}\nstderr: {result.stderr}"
        )


def parse_observables(path: Path):
    """Lee 'resources/observables.txt': una linea por step, formato 't va s'."""
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


def compute_steady_state_stats(va_vals: np.ndarray, s_vals: np.ndarray, burn_in_fraction: float):
    """
    Promedia va y S sobre la 'cola' de la serie temporal (asumiendo que para
    ese entonces el sistema ya llego al estacionario). Es una aproximacion
    simple; si el punto (b) del TP requiere un criterio mas fino (ej. deteccion
    automatica de cuando se estabiliza la varianza), esta funcion es donde
    habria que reemplazar la logica.
    """
    n = len(va_vals)
    start = int(n * burn_in_fraction)
    va_window = va_vals[start:]
    s_window = s_vals[start:]
    return (
        float(np.mean(va_window)), float(np.std(va_window)),
        float(np.mean(s_window)), float(np.std(s_window)),
    )


def main():
    if not EXECUTABLE.exists():
        print(f"No se encontro el ejecutable en {EXECUTABLE}. Compila el proyecto primero.", file=sys.stderr)
        sys.exit(1)

    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    rows = []
    total_runs = len(MODELS) * len(DENSITIES) * len(NOISES) * SEEDS_PER_POINT
    run_count = 0

    for model in MODELS:
        for density in DENSITIES:
            for noise in NOISES:
                for seed in range(SEEDS_PER_POINT):
                    run_count += 1
                    print(f"[{run_count}/{total_runs}] model={model} density={density} noise={noise:.2f} seed={seed}")

                    run_simulation(model, density, noise, STEPS)
                    _, va_vals, s_vals = parse_observables(OBSERVABLES_PATH)
                    va_mean, va_std, s_mean, s_std = compute_steady_state_stats(va_vals, s_vals, BURN_IN_FRACTION)

                    rows.append({
                        "model": model,
                        "density": density,
                        "noise": noise,
                        "seed": seed,
                        "va_mean": va_mean,
                        "va_std": va_std,
                        "s_mean": s_mean,
                        "s_std": s_std,
                    })

    output_path = Path("resources/sweep_summary.csv")
    with open(output_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print(f"\nListo. Resumen guardado en {output_path}")


if __name__ == "__main__":
    main()
