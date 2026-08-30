# TP2 - Simulación de Sistemas: Autómatas Celulares (Off-Lattice)

Implementación del modelo de bandadas de agentes autopropulsados (Vicsek et al., 1995)
y su variante de votante (Loscar, Baglietto & Vazquez, 2021), sobre una caja cuadrada
con condiciones periódicas de contorno.

## Estructura del proyecto

```
ss-tp2/
├── CMakeLists.txt
├── apps/cellular_automata/       # main.cpp + CMakeLists.txt del ejecutable
├── include/                      # headers (.h)
│   ├── board_generation/         # Particle, Tablero (reusado de TP1)
│   ├── cell_index_method/        # ParticleSystem, CIM (reusado de TP1)
│   ├── cellular_automata/        # CellularAutomataSystem, VicsekRule, VoterRule
│   └── utils/                    # generadores de numeros aleatorios
├── src/                          # implementaciones (.cpp), misma estructura que include/
├── resources/                    # output de las simulaciones (frames, observables, csv)
└── scripts/                      # scripts de Python (animacion, graficos, sweep)
```

## Requisitos

**C++:**
- CMake >= 3.16
- Compilador con soporte C++17 (GCC 13 probado en WSL/Linux)

**Python** (para las visualizaciones y el sweep de parámetros):
```bash
pip install numpy pandas matplotlib
```

> **Nota sobre entorno:** todo el pipeline (compilar, correr el sweep, generar gráficos)
> tiene que correr en el **mismo entorno** — si compilás en WSL/Linux, corré también
> los scripts de Python desde WSL con `python3` de Linux, no con el Python nativo de
> Windows. Mezclar ambos rompe `subprocess.run(...)` porque el binario compilado
> no es ejecutable por el otro sistema operativo.

## 1. Compilar

Desde la raíz del proyecto:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
El ejecutable queda en `build/apps/cellular_automata/cellular_automata_app`.

Usar `Release` es importante para el sweep completo — en modo Debug (default de
muchos IDEs) las corridas van a tardar bastante más de lo necesario.

## 2. Correr una simulación individual

**Siempre desde la raíz del proyecto** (no desde `build/`), porque el binario escribe
a `resources/frames.txt` y `resources/observables.txt` con paths relativos:

```bash
./build/apps/cellular_automata/cellular_automata_app \
    --model vicsek \
    --length 10 \
    --density 4 \
    --radius 1.0 \
    --noise 0.5 \
    --steps 2000
```

### Flags disponibles

| Flag           | Descripción                                             | Default   |
|----------------|----------------------------------------------------------|-----------|
| `--model`      | `vicsek` o `voter`                                        | `vicsek`  |
| `--length`     | Lado de la caja (L)                                        | `10`      |
| `--density`    | Densidad ρ = N/L² (usar esto **o** `--particles`, no ambos)| `2.0`     |
| `--particles`  | Cantidad de partículas (N) directamente                    | —         |
| `--radius`     | Radio de interacción (r_c)                                 | `1.0`     |
| `--noise`      | Amplitud de ruido (η)                                      | `0.1`     |
| `--steps`      | Cantidad de pasos de simulación                            | `1000`    |
| `--help`       | Muestra la ayuda                                            | —         |

Salida generada en `resources/`:
- `frames.txt`: posición y ángulo de cada partícula, por paso (para animar).
- `observables.txt`: `t v_a S` por paso (para graficar evolución temporal).

## 3. Ver una animación

```bash
python3 scripts/cellular_automata_visualizer.py resources/frames.txt
```
Para guardar en vez de mostrar en pantalla (requiere `ffmpeg` para `.mp4`, o solo
`pillow` — que ya viene con matplotlib — para `.gif`):
```bash
python3 scripts/cellular_automata_visualizer.py resources/frames.txt salida.gif
```

## 4. Ver evolución temporal de los observables (v_a, S)

Corré primero una simulación (paso 2), después:
```bash
python3 scripts/cellular_automata_observables.py resources/observables.txt
```
Muestra en pantalla dos gráficos (v_a y S vs tiempo) con una línea vertical marcando
dónde el script detectó el inicio del régimen estacionario, e imprime por consola el
promedio ± desvío de cada observable en esa ventana. Para guardar los gráficos en vez
de mostrarlos:
```bash
python3 scripts/cellular_automata_observables.py resources/observables.txt resources/mi_corrida
```
(genera `resources/mi_corrida_va_evolution.png` y `resources/mi_corrida_s_evolution.png`)

## 5. Correr el barrido completo de parámetros (sweep)

Este paso lanza el ejecutable muchas veces (todas las combinaciones de modelo,
densidad, ruido y semillas configuradas dentro del script) y arma un `.csv` resumen.

**Antes de lanzarlo completo**, medí cuánto tarda una corrida individual con el
`--steps` que pensás usar, para estimar el tiempo total:
```bash
time ./build/apps/cellular_automata/cellular_automata_app --model vicsek --density 4 --noise 0.5 --steps 2000
```

Los parámetros del barrido (densidades, rango de η, cantidad de semillas, steps)
están hardcodeados al principio de `run_cellular_automata_sweep.py` — ajustalos ahí
antes de correr si hace falta.

```bash
python3 scripts/run_cellular_automata_sweep.py
```

Esto es **serial, no paralelo** (el binario siempre escribe a los mismos archivos
fijos en `resources/`, así que dos corridas simultáneas se pisarían entre sí).
Puede tardar bastante según cuántas combinaciones tengas configuradas — dejalo
corriendo y no lo interrumpas a mitad de camino, o vas a tener el `.csv` incompleto.

Al terminar, genera `resources/sweep_summary.csv`.

## 6. Generar los gráficos finales del barrido

```bash
python3 scripts/cellular_automata_sweep_plot.py resources/sweep_summary.csv resources/graficos
```
Genera en `resources/graficos/`:
- `va_vs_eta_<modelo>.png` — parámetro de orden vs ruido, por densidad.
- `s_vs_eta_<modelo>.png` — fracción del cluster más grande vs ruido, por densidad.
- `va_vs_s_<modelo>.png` — scatter v_a vs S, por densidad.
- `va_vs_eta_comparison_rho<N>.png` / `s_vs_eta_comparison_rho<N>.png` — comparación
  Vicsek vs Votante, para cada densidad.

Si se omite el segundo argumento, los gráficos se muestran en pantalla en vez de
guardarse a archivo.

## Orden recomendado para armar todo el TP

1. Compilar (paso 1).
2. Correr 2-3 simulaciones cortas variando η a mano y confirmar con
   `cellular_automata_observables.py` que v_a se comporta como se espera
   (alto a η bajo, bajo a η alto) — chequeo de sanity antes de gastar tiempo
   en el sweep completo.
3. Elegir 1-2 corridas "características" por densidad para animar (punto a del TP)
   y guardarlas aparte (copiá `resources/frames.txt` a otro nombre antes de
   correr la siguiente simulación, porque se pisa).
4. Correr el sweep completo (paso 5) — puede tardar, dejalo corriendo tranquilo.
5. Generar los gráficos finales (paso 6).
6. Repetir 3-5 para el modelo de votante si no lo corriste ya en el mismo sweep.

## Problemas conocidos / limitaciones actuales

- El binario no permite elegir el nombre de los archivos de salida — corridas
  sucesivas pisan `resources/frames.txt` y `resources/observables.txt`. Si querés
  conservar una corrida particular, copiá el archivo a otro nombre antes de
  correr la siguiente.
- No hay forma de fijar semilla del generador aleatorio desde la línea de comandos
  — cada corrida usa una semilla distinta automáticamente (útil para "varias
  realizaciones", pero no reproducible bit a bit si necesitás repetir *exactamente*
  la misma corrida).
- El punto (g) del TP (comparación de tiempos de CIM contra TP1) no tiene un modo
  dedicado en `main.cpp` todavía.
