#!/usr/bin/env bash
# Genera los resultados experimentales requeridos para el TP2.
# Uso desde WSL, parado en cualquier directorio:
#   bash scripts/generar_entregables_tp2.sh
#   bash scripts/generar_entregables_tp2.sh resultados_tp2

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

OUTPUT_DIR="${1:-tp2_resultados}"
if [[ "$OUTPUT_DIR" != /* ]]; then
    OUTPUT_DIR="$ROOT/$OUTPUT_DIR"
fi

PYTHON_BIN="${PYTHON_BIN:-python3}"
STEPS="${STEPS:-2000}"
REPETITIONS="${REPETITIONS:-30}"
CHARACTERISTIC_NOISE="${CHARACTERISTIC_NOISE:-1}"
# Con 2000 pasos: stride 10 produce 200 cuadros, de los cuales alrededor de
# la mitad corresponden al régimen estacionario. 100 ms da un GIF legible.
ANIMATION_STRIDE="${ANIMATION_STRIDE:-10}"
ANIMATION_INTERVAL_MS="${ANIMATION_INTERVAL_MS:-100}"
EXECUTABLE="$ROOT/build/apps/cellular_automata/cellular_automata_app"
if [[ -z "${TP1_ROOT:-}" ]]; then
    TP1_ROOT="$ROOT/tp1"
    if [[ ! -d "$TP1_ROOT" ]]; then
        TP1_ROOT="$ROOT/../ss-tp1"
    fi
fi
TP1_RUNS="${TP1_RUNS:-5}"
TP1_WARMUP="${TP1_WARMUP:-30}"

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Falta '$1'. Instalarlo en WSL antes de continuar." >&2
        exit 1
    }
}

require_command cmake
require_command g++
require_command "$PYTHON_BIN"
[[ -d "$TP1_ROOT" ]] || {
    echo "No se encontro TP1 en $TP1_ROOT. Indicar TP1_ROOT=/ruta/a/ss-tp1." >&2
    exit 1
}
"$PYTHON_BIN" -c 'import numpy, pandas, matplotlib, PIL' || {
    echo "Instala las dependencias: python3 -m pip install numpy pandas matplotlib pillow" >&2
    exit 1
}

# Se toma la lista desde el protocolo configurado en el script de sweep, para
# que las corridas características y sus gráficas cubran exactamente las mismas
# densidades (incluidas las adicionales indicadas por la cátedra).
mapfile -t DENSITIES < <("$PYTHON_BIN" -c \
    "import runpy; config = runpy.run_path('scripts/run_cellular_automata_sweep.py'); print(*config['DENSITIES'], sep='\\n')")

mkdir -p "$OUTPUT_DIR"/{animaciones,evoluciones,figuras,benchmark,datos_caracteristicos}

echo "==> Compilando en modo Release"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

echo "==> Barrido completo: Vicsek y votante; densidades configuradas en run_cellular_automata_sweep.py"
echo "    El barrido usa 5 realizaciones, 11 valores de eta y $STEPS pasos por corrida."
# El script de sweep contiene los parametros del protocolo. STEPS se aplica
# temporalmente para que una corrida de este wrapper no requiera editarlo.
sed "s/^STEPS = .*/STEPS = $STEPS/" scripts/run_cellular_automata_sweep.py > "$OUTPUT_DIR/run_sweep.py"
export TP2_SWEEP_OUTPUT="$OUTPUT_DIR/sweep_temporal"
"$PYTHON_BIN" "$OUTPUT_DIR/run_sweep.py"
unset TP2_SWEEP_OUTPUT
cp resources/sweep_summary.csv "$OUTPUT_DIR/sweep_summary.csv"

echo "==> Graficos estacionarios: va(eta), S(eta) y va(S)"
"$PYTHON_BIN" scripts/cellular_automata_sweep_plot.py \
    "$OUTPUT_DIR/sweep_summary.csv" "$OUTPUT_DIR/figuras"

run_characteristic_case() {
    local model="$1"
    local density="$2"
    local case_dir="$OUTPUT_DIR/datos_caracteristicos/${model}_rho${density}_eta${CHARACTERISTIC_NOISE}"
    mkdir -p "$case_dir"

    "$EXECUTABLE" --model "$model" --length 10 --density "$density" \
        --radius 1.0 --noise "$CHARACTERISTIC_NOISE" --steps "$STEPS" \
        --output "$case_dir"

    # Se toma un frame cada ANIMATION_STRIDE pasos para que el GIF sea utilizable.
    "$PYTHON_BIN" scripts/cellular_automata_visualizer.py \
        "$case_dir/frames.txt" \
        "$OUTPUT_DIR/animaciones/${model}_rho${density}.gif" \
        --stride "$ANIMATION_STRIDE" --interval "$ANIMATION_INTERVAL_MS"
}

echo "==> Corridas caracteristicas, evoluciones temporales y animaciones"
for model in vicsek voter; do
    for density in "${DENSITIES[@]}"; do
        run_characteristic_case "$model" "$density"
    done
done

echo "==> Evoluciones temporales conjuntas por densidad"
for model in vicsek voter; do
    temporal_args=("$OUTPUT_DIR/evoluciones/${model}")
    for density in "${DENSITIES[@]}"; do
        temporal_args+=(--series "$density" \
            "$OUTPUT_DIR/datos_caracteristicos/${model}_rho${density}_eta${CHARACTERISTIC_NOISE}/observables.txt")
    done
    "$PYTHON_BIN" scripts/cellular_automata_temporal_by_density.py "${temporal_args[@]}"
done

echo "==> Comparacion temporal directa: Vicsek vs votante"
comparison_args=("$OUTPUT_DIR/evoluciones/comparacion_modelos")
for model in vicsek voter; do
    for density in "${DENSITIES[@]}"; do
        comparison_args+=(--series "$model" "$density" \
            "$OUTPUT_DIR/datos_caracteristicos/${model}_rho${density}_eta${CHARACTERISTIC_NOISE}/observables.txt")
    done
done
"$PYTHON_BIN" scripts/cellular_automata_temporal_model_comparison.py "${comparison_args[@]}"

echo "==> Evoluciones temporales combinadas: v_a(t) y S(t), una figura por densidad"
per_density_args=("$OUTPUT_DIR/evoluciones/por_densidad")
for density in "${DENSITIES[@]}"; do
    for model in vicsek voter; do
        per_density_args+=(--series "$density" "$model" \
            "$OUTPUT_DIR/datos_caracteristicos/${model}_rho${density}_eta${CHARACTERISTIC_NOISE}/observables.txt")
    done
done
"$PYTHON_BIN" scripts/cellular_automata_temporal_per_density.py "${per_density_args[@]}"

echo "==> Benchmark comparativo: TP1 desde cero, CIM TP2 y fuerza bruta TP2"
TP1_ROOT="$TP1_ROOT" PYTHON_BIN="$PYTHON_BIN" REPETITIONS="$TP1_RUNS" WARMUP="$TP1_WARMUP" \
    bash scripts/generar_benchmark_comparativo.sh "$OUTPUT_DIR/benchmark"

cat <<EOF

Resultados generados en: $OUTPUT_DIR
  - Benchmark comparativo: $OUTPUT_DIR/benchmark/comparacion_tiempos_busqueda.png

Todavia queda trabajo de presentacion/informe:
  - elegir cuales GIFs y evoluciones temporales son las mas representativas;
  - incluir las figuras en ambos documentos, con leyendas, ejes y conclusiones;
  - exportar el PDF de presentacion, el PDF de informe y el ZIP final de codigo.
EOF
