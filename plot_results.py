#!/usr/bin/env python3
"""
plot_results.py
Gera gráficos de Speedup e Eficiência a partir dos resultados dos experimentos.

Uso:
    python3 plot_results.py results/resultados_todos.csv

Saída:
    plots/speedup_pi.png
    plots/speedup_integral.png
    plots/eficiencia_pi.png
    plots/eficiencia_integral.png
    plots/tempo_threads.png
    plots/erro_vs_N.png
"""

import sys
import os
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Estilo ────────────────────────────────────────────────────────────────────
plt.rcParams.update({
    'font.family': 'DejaVu Sans',
    'font.size': 12,
    'axes.titlesize': 14,
    'axes.labelsize': 13,
    'legend.fontsize': 11,
    'xtick.labelsize': 11,
    'ytick.labelsize': 11,
    'figure.dpi': 150,
    'axes.grid': True,
    'grid.alpha': 0.4,
    'lines.linewidth': 2,
    'lines.markersize': 7,
})

CORES = [1, 2, 4, 8, 10, 20, 40]
COLORS = plt.cm.tab10.colors

os.makedirs('plots', exist_ok=True)

# ── Leitura dos dados ─────────────────────────────────────────────────────────
if len(sys.argv) < 2:
    print("Uso: python3 plot_results.py <arquivo_csv>")
    sys.exit(1)

csv_path = sys.argv[1]
df = pd.read_csv(csv_path)

print(f"[INFO] {len(df)} linhas carregadas de '{csv_path}'")
print(df.head())

# ── Pré-processamento ─────────────────────────────────────────────────────────
# Média e desvio padrão por (tipo, N, threads)
agg = df.groupby(['tipo', 'N', 'threads'])['tempo_s'].agg(['mean', 'std']).reset_index()
agg.columns = ['tipo', 'N', 'threads', 'tempo_medio', 'tempo_std']

SIZES = sorted(df['N'].unique())
TIPOS = df['tipo'].unique()

# ── Helper: formata N em notação legível ──────────────────────────────────────
def fmt_N(n):
    if n >= 1e9:
        return f'{n/1e9:.0f}×10⁹'
    elif n >= 1e6:
        return f'{n/1e6:.0f}×10⁶'
    return str(n)

# ── Calcula Speedup e Eficiência ──────────────────────────────────────────────
rows = []
for tipo in TIPOS:
    for N in SIZES:
        sub = agg[(agg['tipo'] == tipo) & (agg['N'] == N)]
        # Tempo sequencial = tempo com 1 thread
        t_seq_row = sub[sub['threads'] == 1]
        if t_seq_row.empty:
            continue
        t_seq = t_seq_row['tempo_medio'].values[0]

        for _, row in sub.iterrows():
            t = row['tempo_medio']
            t_std = row['tempo_std']
            threads = row['threads']
            speedup = t_seq / t if t > 0 else np.nan
            efic = speedup / threads if threads > 0 else np.nan
            rows.append({
                'tipo': tipo, 'N': N, 'threads': threads,
                'tempo_medio': t, 'tempo_std': t_std,
                'speedup': speedup, 'eficiencia': efic,
            })

perf = pd.DataFrame(rows)

# ── 1. Gráficos de Speedup por tipo ──────────────────────────────────────────
for tipo in TIPOS:
    fig, ax = plt.subplots(figsize=(8, 5))

    sub = perf[perf['tipo'] == tipo]

    for i, N in enumerate(SIZES):
        sub_n = sub[sub['N'] == N].sort_values('threads')
        if sub_n.empty:
            continue
        ax.plot(sub_n['threads'], sub_n['speedup'],
                marker='o', color=COLORS[i % len(COLORS)],
                label=f'N = {fmt_N(N)}')

    # Linha de speedup ideal
    ax.plot(CORES, CORES, 'k--', linewidth=1.5, label='Ideal (linear)', alpha=0.6)

    ax.set_xlabel('Número de Threads')
    ax.set_ylabel('Speedup')
    ax.set_title(f'Speedup — Monte Carlo ({tipo})')
    ax.set_xticks(CORES)
    ax.set_xlim(0, max(CORES) + 1)
    ax.set_ylim(0)
    ax.legend(loc='upper left')
    fig.tight_layout()
    path = f'plots/speedup_{tipo}.png'
    fig.savefig(path)
    plt.close(fig)
    print(f'[OK] {path}')

# ── 2. Gráficos de Eficiência por tipo ───────────────────────────────────────
for tipo in TIPOS:
    fig, ax = plt.subplots(figsize=(8, 5))

    sub = perf[perf['tipo'] == tipo]

    for i, N in enumerate(SIZES):
        sub_n = sub[sub['N'] == N].sort_values('threads')
        if sub_n.empty:
            continue
        ax.plot(sub_n['threads'], sub_n['eficiencia'] * 100,
                marker='s', color=COLORS[i % len(COLORS)],
                label=f'N = {fmt_N(N)}')

    ax.axhline(100, color='k', linestyle='--', linewidth=1.5,
               label='Ideal (100%)', alpha=0.6)

    ax.set_xlabel('Número de Threads')
    ax.set_ylabel('Eficiência (%)')
    ax.set_title(f'Eficiência — Monte Carlo ({tipo})')
    ax.set_xticks(CORES)
    ax.set_xlim(0, max(CORES) + 1)
    ax.set_ylim(0, 110)
    ax.legend(loc='upper right')
    fig.tight_layout()
    path = f'plots/eficiencia_{tipo}.png'
    fig.savefig(path)
    plt.close(fig)
    print(f'[OK] {path}')

# ── 3. Tempo × Threads (escala log) para o maior N ───────────────────────────
fig, axes = plt.subplots(1, len(TIPOS), figsize=(6 * len(TIPOS), 5), sharey=False)
if len(TIPOS) == 1:
    axes = [axes]

for ax, tipo in zip(axes, TIPOS):
    N_max = max(SIZES)
    sub = perf[(perf['tipo'] == tipo) & (perf['N'] == N_max)].sort_values('threads')

    ax.errorbar(sub['threads'], sub['tempo_medio'],
                yerr=sub['tempo_std'],
                marker='D', color='steelblue', capsize=4, label='Medido')

    ax.set_xlabel('Número de Threads')
    ax.set_ylabel('Tempo (s)')
    ax.set_title(f'Tempo × Threads\n({tipo}, N={fmt_N(N_max)})')
    ax.set_xticks(CORES)
    ax.legend()

fig.tight_layout()
path = 'plots/tempo_threads.png'
fig.savefig(path)
plt.close(fig)
print(f'[OK] {path}')

# ── 4. Erro relativo × N (convergência estatística) ──────────────────────────
fig, ax = plt.subplots(figsize=(8, 5))

for tipo in TIPOS:
    sub = df[df['threads'] == 1].groupby(['tipo', 'N'])['erro_relativo'].mean().reset_index()
    sub_t = sub[sub['tipo'] == tipo].sort_values('N')
    ax.loglog(sub_t['N'], sub_t['erro_relativo'],
              marker='o', label=tipo)

# Linha de referência 1/sqrt(N)
N_ref = np.array(sorted(SIZES), dtype=float)
ref_err = 1.0 / np.sqrt(N_ref)
ref_err = ref_err / ref_err[0] * sub_t['erro_relativo'].values[0]
ax.loglog(N_ref, ref_err, 'k--', alpha=0.6, label='O(1/√N) teórico')

ax.set_xlabel('N (número de amostras)')
ax.set_ylabel('Erro Relativo Médio')
ax.set_title('Convergência do Método de Monte Carlo')
ax.legend()
fig.tight_layout()
path = 'plots/erro_vs_N.png'
fig.savefig(path)
plt.close(fig)
print(f'[OK] {path}')

# ── Tabela resumo no terminal ─────────────────────────────────────────────────
print('\n=== TABELA DE SPEEDUP (tipo=pi, N máximo) ===')
N_max = max(SIZES)
tab = perf[(perf['tipo'] == 'pi') & (perf['N'] == N_max)][
    ['threads', 'tempo_medio', 'speedup', 'eficiencia']
].sort_values('threads')
tab = tab.rename(columns={
    'threads': 'Threads',
    'tempo_medio': 'Tempo (s)',
    'speedup': 'Speedup',
    'eficiencia': 'Eficiência'
})
tab['Eficiência (%)'] = (tab['Eficiência'] * 100).round(1)
print(tab[['Threads', 'Tempo (s)', 'Speedup', 'Eficiência (%)']].to_string(index=False))
print('\n[INFO] Todos os gráficos salvos em plots/')