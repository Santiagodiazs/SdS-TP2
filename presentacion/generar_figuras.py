"""Regenera las figuras de la presentacion desde tp2_resultados/ (no re-simula).

Estilo segun GuiaPresentaciones: sin titulos, ejes en palabras, fuente >= 20,
coma decimal y potencias de 10 como supraindice. Las figuras se guardan al
tamano en que se muestran en la diapositiva para que la fuente sea 1:1.

Uso (desde la raiz del repo): venv/bin/python presentacion/generar_figuras.py
"""
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.ticker as mticker
import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
RES = ROOT / "tp2_resultados"
OUT = Path(__file__).resolve().parent / "figuras"
OUT.mkdir(exist_ok=True)

plt.rcParams.update({
    "font.size": 20, "axes.labelsize": 20, "xtick.labelsize": 20, "ytick.labelsize": 20,
    "legend.fontsize": 18, "mathtext.fontset": "dejavusans", "lines.markersize": 5,
    "axes.formatter.use_mathtext": True,
})
W, H = 6.0, 5.0  # area de ejes; con la leyenda externa ocupa ~2/3 del ancho de la diapositiva
DENS = [1 / (3 * math.pi), 1 / (2 * math.pi), 1 / math.pi, 2.0, 4.0, 8.0]
DENS_DIR = ["0.1061032953945969", "0.15915494309189535", "0.3183098861837907", "2", "4", "8"]
COLORS = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b"]
MARKERS = ["o", "s", "^", "D", "v", "P"]
MODEL_NAME = {"vicsek": "Vicsek", "voter": "Votante"}
SEEDS = 5


def t_estacionario(eta):
    """Inicio del estacionario elegido a ojo sobre las series de tp2_resultados/series:
    con eta >= 1 rad todos los casos se estabilizan antes de t ~ 300; con eta <= 0,5 rad
    algunos (baja densidad, votante sin ruido) recien cerca de t ~ 1000."""
    return 1000 if eta <= 0.5 else 300


def comma(x, _=None):
    s = f"{x:g}"
    return s.replace(".", ",")


def rho_label(r):
    # densidades bajas como fracciones de pi (mas corto y exacto que el decimal)
    for k, s in [(1, r"1/\pi"), (2, r"1/(2\pi)"), (3, r"1/(3\pi)")]:
        if math.isclose(r, 1 / (k * math.pi)):
            return f"${s}$"
    return f"{r:g}"


def lin_axes(ax):
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(comma))
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(comma))


def save(fig, name):
    fig.savefig(OUT / name, dpi=200, bbox_inches="tight")
    plt.close(fig)


def load_obs(model, idx, eta="1"):
    d = RES / "datos_caracteristicos" / f"{model}_rho{DENS_DIR[idx]}_eta{eta}" / "observables.txt"
    return np.loadtxt(d)


BAJAS, ALTAS = [0, 1, 2], [3, 4, 5]  # indices en DENS


def pair(name, draw, xlabel, ylabel):
    """Una figura por modelo con los mismos ejes, para ubicarlas lado a lado en la diapositiva
    (Vicsek | Votante). Sin titulos: el modelo se indica en la diapositiva."""
    for model in ("vicsek", "voter"):
        fig, ax = plt.subplots(figsize=(5.0, 4.6))
        draw(ax, model)
        ax.set_xlabel(xlabel)
        if model == "vicsek":
            ax.set_ylabel(ylabel)
        else:
            ax.legend(title=r"$\rho$", loc="center left", bbox_to_anchor=(1.01, 0.5), frameon=False)
        lin_axes(ax)
        save(fig, f"{name}_{model}.png")


# ---------- evoluciones temporales (eta = 1) ----------
def evolutions():
    for col, ylabel, name, idxs in [(1, "$v_a$", "evol_va", ALTAS),
                                    (2, "$S$", "evol_s_altas", ALTAS),
                                    (2, "$S$", "evol_s_bajas", BAJAS)]:
        def draw(ax, model):
            for idx in idxs:
                o = load_obs(model, idx)
                ax.plot(o[:, 0], o[:, col], color=COLORS[idx], lw=1.2, label=rho_label(DENS[idx]))
            ax.axvline(t_estacionario(1.0), color="k", ls="--", lw=1.5)
            ax.set_xlim(0, 2000)
            ax.set_xticks([0, 1000, 2000])
            ax.set_ylim(0, 1.05)
        pair(name, draw, "Paso temporal", ylabel)


# ---------- observables estacionarios ----------
def sweep_stats():
    # promedio temporal de cada realizacion desde t_e; luego promedio y desvio ENTRE realizaciones
    rows = []
    for model in ("vicsek", "voter"):
        for r, tag in zip(DENS, DENS_DIR):
            for k in range(11):
                eta = 0.5 * k
                for seed in range(SEEDS):
                    o = np.loadtxt(RES / "series" / f"{model}_rho{tag}_eta{eta:g}_s{seed}" / "observables.txt")
                    w = o[o[:, 0] >= t_estacionario(eta)]
                    rows.append((model, r, eta, seed, w[:, 1].mean(), w[:, 2].mean()))
    d = pd.DataFrame(rows, columns=["model", "density", "noise", "seed", "va_mean", "s_mean"])
    return d.groupby(["model", "density", "noise"]).agg(
        va=("va_mean", "mean"), va_err=("va_mean", "std"),
        s=("s_mean", "mean"), s_err=("s_mean", "std"), n=("seed", "count")).reset_index()


def vs_eta(g):
    for col, ylabel, name, idxs, ylim in [
            ("va", "$v_a$", "va_vs_eta", BAJAS + ALTAS, (0, 1.08)),
            ("s", "$S$", "s_vs_eta_bajas", BAJAS, (0, 1.08)),
            ("s", "$S$", "s_vs_eta_altas", ALTAS, (0.94, 1.005))]:
        def draw(ax, model):
            for i in idxs:
                m = g[(g.model == model) & np.isclose(g.density, DENS[i])]
                ax.errorbar(m.noise, m[col], yerr=m[col + "_err"], fmt=MARKERS[i] + "-",
                            color=COLORS[i], lw=1, capsize=3, label=rho_label(DENS[i]))
            ax.set_ylim(*ylim)
        pair(name, draw, "Ruido (rad)", ylabel)


def va_vs_s(g):
    # limites con margen para que los rotulos de la esquina (x minimo, y = 0) no se encimen
    for name, idxs, xlim in [("va_vs_s_bajas", BAJAS, (-0.05, 1.1)), ("va_vs_s_altas", ALTAS, (0.935, 1.005))]:
        def draw(ax, model):
            for i in idxs:
                m = g[(g.model == model) & np.isclose(g.density, DENS[i])]
                ax.errorbar(m.s, m.va, xerr=m.s_err, yerr=m.va_err, fmt=MARKERS[i], color=COLORS[i],
                            elinewidth=0.8, capsize=3, label=rho_label(DENS[i]))
            ax.set_xlim(*xlim)
            ax.set_ylim(-0.05, 1.1)
        pair(name, draw, "$S$", "$v_a$")


# ---------- benchmark ----------
def benchmark():
    b = RES / "benchmark"
    tp1 = pd.read_csv(b / "variacion_N_densidad_libre_tp1.csv")
    tp1 = tp1[tp1.algorithm == "CELL_INDEX_METHOD"].sort_values("N")
    tp2 = pd.read_csv(b / "cim_tp2.csv")
    tp2["ms"] = tp2.time_ns / 1e6
    tp2 = tp2.groupby(["algorithm", "N"]).ms.agg(["mean", "std"]).reset_index()
    fig, ax = plt.subplots(figsize=(8.3, H))
    ax.errorbar(tp1.N, tp1.mean_ms, yerr=tp1.std_ms, fmt="s--", color="#d62728", capsize=3, label="CIM (TP1)")
    for alg, lab, c, mk in [("CIM", "CIM (TP2)", "#1f77b4", "o-"), ("BRUTE_FORCE", "Fuerza bruta (TP2)", "0.4", "^-")]:
        m = tp2[tp2.algorithm == alg]
        ax.errorbar(m.N, m["mean"], yerr=m["std"], fmt=mk, color=c, capsize=3, label=lab)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("$N$")
    ax.set_ylabel("Tiempo de búsqueda (ms)")
    ax.legend(loc="upper left", frameon=False)
    save(fig, "benchmark.png")


# ---------- animaciones (mp4) y fotograma representativo ----------
def parse_frames(path):
    frames, cur = [], []
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                if cur:
                    frames.append(np.array(cur))
                    cur = []
                continue
            cur.append([float(v) for v in p])
    if cur:
        frames.append(np.array(cur))
    return frames


def animations(stride=10, fps=10):
    cases = [(idx, "1", f"rho{'baja' if idx == 0 else 'alta'}") for idx in (0, 5)]
    cases += [(4, e, f"eta{lab}") for e, lab in [("0.5", "baja"), ("4.5", "alta")]]
    for model in ("vicsek", "voter"):
        for idx, eta, tag in cases:
            frames = parse_frames(RES / "datos_caracteristicos" / f"{model}_rho{DENS_DIR[idx]}_eta{eta}" / "frames.txt")[::stride]
            fig, ax = plt.subplots(figsize=(6, 6))
            ax.set_xlim(0, 10)
            ax.set_ylim(0, 10)
            ax.set_aspect("equal")
            ax.set_xticks([])
            ax.set_yticks([])
            f0 = frames[0]
            q = ax.quiver(f0[:, 0], f0[:, 1], np.cos(f0[:, 2]), np.sin(f0[:, 2]),
                          np.mod(f0[:, 2] + np.pi, 2 * np.pi) - np.pi, cmap="hsv", clim=(-np.pi, np.pi),
                          scale=25 if idx == 0 else 40, width=0.006 if idx == 0 else 0.003)
            txt = ax.text(0.02, 1.01, "", transform=ax.transAxes, fontsize=20, va="bottom")

            def update(k):
                f = frames[k]
                q.set_offsets(f[:, :2])
                q.set_UVC(np.cos(f[:, 2]), np.sin(f[:, 2]), np.mod(f[:, 2] + np.pi, 2 * np.pi) - np.pi)
                txt.set_text(f"Paso {(k + 1) * stride}")
                return q, txt

            name = f"anim_{model}_{tag}"
            anim = animation.FuncAnimation(fig, update, frames=len(frames))
            anim.save(OUT / f"{name}.mp4", writer="ffmpeg", fps=fps, dpi=120)
            update(len(frames) - 1)  # poster: ultimo fotograma (regimen estacionario)
            fig.savefig(OUT / f"{name}.png", dpi=150, bbox_inches="tight")
            plt.close(fig)


if __name__ == "__main__":
    evolutions()
    g = sweep_stats()
    g.to_csv(OUT / "resumen_estacionario.csv", index=False)
    vs_eta(g)
    va_vs_s(g)
    benchmark()
    animations()
    print("Figuras en", OUT)
