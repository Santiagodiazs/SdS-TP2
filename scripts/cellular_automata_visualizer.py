"""
cellular_automata_visualizer.py

Anima un archivo de frames generado por CellularAutomataSystem::writeFrame()
(formato: por cada frame, una linea "x y angle" por particula, y una linea
en blanco separando frames).

Cada particula se dibuja como un vector (quiver) con origen en su posicion,
coloreado segun el angulo de su velocidad (colormap ciclico, porque el
angulo es periodico: 0 y 2*pi son el mismo color).

Uso:
    python cellular_automata_visualizer.py resources/frames.txt [salida.mp4] [--stride N]
"""

import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

LENGTH = 10  # tiene que coincidir con el --length usado al generar la corrida


def parse_frames(path: Path):
    """Devuelve una lista de frames, cada uno un array Nx3 (x, y, angle)."""
    frames = []
    current = []

    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                if current:
                    frames.append(np.array(current))
                    current = []
                continue
            x, y, angle = line.split()
            current.append((float(x), float(y), float(angle)))

    if current:
        frames.append(np.array(current))

    return frames


def animate_frames(frames, length, output_path=None, interval_ms=50):
    fig, ax = plt.subplots(figsize=(6, 6))
    ax.set_xlim(0, length)
    ax.set_ylim(0, length)
    ax.set_aspect("equal")
    ax.set_title("Bandadas de agentes autopropulsados")

    first = frames[0]
    x, y, angle = first[:, 0], first[:, 1], first[:, 2]
    u, v = np.cos(angle), np.sin(angle)

    quiver = ax.quiver(x, y, u, v, angle, cmap="hsv", clim=(-np.pi, np.pi),
                        scale=25, width=0.004)

    def update(frame_idx):
        frame = frames[frame_idx]
        x, y, angle = frame[:, 0], frame[:, 1], frame[:, 2]
        u, v = np.cos(angle), np.sin(angle)
        quiver.set_offsets(np.column_stack([x, y]))
        quiver.set_UVC(u, v, angle)
        ax.set_xlabel(f"frame {frame_idx}/{len(frames) - 1}")
        return quiver,

    anim = animation.FuncAnimation(fig, update, frames=len(frames),
                                    interval=interval_ms, blit=False)

    if output_path:
        # Requiere ffmpeg instalado para .mp4, o pillow para .gif.
        writer = "ffmpeg" if str(output_path).endswith(".mp4") else "pillow"
        anim.save(output_path, writer=writer, fps=1000 // interval_ms)
        print(f"Animacion guardada en: {output_path}")
    else:
        plt.show()

    plt.close(fig)


def main():
    if len(sys.argv) < 2:
        print("Uso: python cellular_automata_visualizer.py <ruta_a_frames.txt> [salida.mp4|salida.gif] [--stride N]")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_path = None
    stride = 1
    arguments = sys.argv[2:]
    if arguments and not arguments[0].startswith("--"):
        output_path = arguments.pop(0)
    if arguments:
        if len(arguments) != 2 or arguments[0] != "--stride":
            print("Argumentos invalidos. Usar --stride N.", file=sys.stderr)
            sys.exit(1)
        stride = int(arguments[1])
        if stride < 1:
            print("El stride debe ser mayor o igual a 1.", file=sys.stderr)
            sys.exit(1)

    frames = parse_frames(input_path)
    print(f"{len(frames)} frames leidos de {input_path}")

    if not frames:
        print("No hay frames para animar.")
        sys.exit(1)

    frames = frames[::stride]
    animate_frames(frames, LENGTH, output_path)


if __name__ == "__main__":
    main()
