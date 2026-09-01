#!/usr/bin/env bash
# Genera desde cero: CIM TP1, CIM TP2 y fuerza bruta TP2.
# Uso: bash scripts/generar_benchmark_comparativo.sh [directorio_salida]

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

OUTPUT_DIR="${1:-tp2_resultados/benchmark}"
[[ "$OUTPUT_DIR" = /* ]] || OUTPUT_DIR="$ROOT/$OUTPUT_DIR"
TP1_ROOT="${TP1_ROOT:-$ROOT/../ss-tp1}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
REPETITIONS="${REPETITIONS:-5}"
WARMUP="${WARMUP:-30}"
# TP1 mide la variacion de N con L=20. Para que la comparacion sea justa,
# TP2 usa la misma caja solo durante el benchmark (las simulaciones del TP
# siguen usando L=10).
BENCHMARK_LENGTH="${BENCHMARK_LENGTH:-20}"
# Se usa el M óptimo determinado por TP1, para medir ambos CIM sobre la
# misma partición espacial. No modifica el M de las simulaciones de TP2.
BENCHMARK_SUBSQUARES="${BENCHMARK_SUBSQUARES:-13}"
TP1_CSV="$ROOT/benchmark_reference/variacion_N_densidad_libre_tp1.csv"
TP2_CSV="$OUTPUT_DIR/cim_tp2.csv"
TP2_EXECUTABLE="$ROOT/build/apps/cellular_automata/cellular_automata_app"

[[ -d "$TP1_ROOT" ]] || { echo "No se encontro TP1 en $TP1_ROOT" >&2; exit 1; }
mkdir -p "$OUTPUT_DIR" "$(dirname "$TP1_CSV")"

echo "==> Compilando y ejecutando el benchmark de TP1"
cmake -S "$TP1_ROOT" -B "$TP1_ROOT/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$TP1_ROOT/build" --parallel
(
    cd "$TP1_ROOT"
    ./build/apps/benchmark/ss_tp1_benchmark --runs "$REPETITIONS" --warmup "$WARMUP"
)
cp "$TP1_ROOT/resources/variacion_N_densidad_libre.csv" "$TP1_CSV"
cp "$TP1_CSV" "$OUTPUT_DIR/variacion_N_densidad_libre_tp1.csv"

echo "==> Compilando TP2"
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build" --parallel

mapfile -t N_VALUES < <("$PYTHON_BIN" -c \
    "import pandas as pd; d = pd.read_csv(r'$TP1_CSV'); print(*sorted(d.N.unique()), sep='\\n')")

: > "$TP2_CSV"
echo "==> Midiendo CIM y fuerza bruta de TP2 para los mismos N, L=$BENCHMARK_LENGTH y M=$BENCHMARK_SUBSQUARES"
for algorithm in cim brute; do
    for particles in "${N_VALUES[@]}"; do
        "$TP2_EXECUTABLE" --benchmark --benchmark-algorithm "$algorithm" \
            --particles "$particles" --length "$BENCHMARK_LENGTH" --radius 1.0 \
            --benchmark-subsquares "$BENCHMARK_SUBSQUARES" \
            --repetitions "$REPETITIONS" --warmup "$WARMUP" --output "$TP2_CSV"
    done
done

"$PYTHON_BIN" scripts/plot_cim_tp1_tp2_comparison.py \
    "$TP1_CSV" "$TP2_CSV" "$OUTPUT_DIR/comparacion_tiempos_busqueda.png"

echo "Listo: $OUTPUT_DIR/comparacion_tiempos_busqueda.png"
