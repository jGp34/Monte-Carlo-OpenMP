#!/bin/bash
#SBATCH --partition=hype
#SBATCH --job-name=mc_vtune
#SBATCH --output=saida_vtune.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=40
#SBATCH --time=02:00:00

# =============================================================================
# Carrega o ambiente do Intel VTune Profiler
# =============================================================================
source /home/intel/oneapi/vtune/2021.1.1/vtune-vars.sh

# =============================================================================
# Compilação com símbolos de debug (necessário para VTune identificar funções)
# =============================================================================
gcc -O2 -g -fopenmp -o monte_carlo_omp_vtune monte_carlo_omp.c -lm

echo "=================================================="
echo " COLETA VTUNE - MONTE CARLO OpenMP"
echo " Node: $SLURMD_NODENAME | $(date)"
echo "=================================================="

# Usamos N grande para que o perfil seja representativo
N=500000000
SEED=42

# Threads a perfilar: sequencial, metade dos cores, todos físicos, com HT
THREADS_VTUNE="1 10 20 40"

mkdir -p vtune_results/summaries

# =============================================================================
# Análise hpc-performance
# Coleta: CPI, uso de cache L1/L2/LLC, bandwidth de memória, vetorização.
# Funciona sem permissões privilegiadas.
# =============================================================================
echo ""
echo ">>> Iniciando hpc-performance"

for T in $THREADS_VTUNE; do
    export OMP_NUM_THREADS=$T
    echo ""
    echo "--- hpc-performance | N=$N | Threads=$T ---"

    # Execução limpa para registrar tempo sem overhead do VTune
    ./monte_carlo_omp_vtune $N $SEED

    vtune -collect hpc-performance \
          -result-dir vtune_results/hpc_N${N}_T${T} \
          ./monte_carlo_omp_vtune $N $SEED > /dev/null 2>&1

    echo "[OK] vtune_results/hpc_N${N}_T${T}"
done

# =============================================================================
# Análise hotspots com software-based sampling
# Modo sw nao exige acesso a contadores de hardware privilegiados.
# =============================================================================
echo ""
echo ">>> Iniciando hotspots (software sampling)"

for T in 1 20; do
    export OMP_NUM_THREADS=$T
    echo ""
    echo "--- hotspots (sw) | N=$N | Threads=$T ---"

    vtune -collect hotspots \
          -knob sampling-mode=sw \
          -result-dir vtune_results/hotspots_sw_N${N}_T${T} \
          ./monte_carlo_omp_vtune $N $SEED > /dev/null 2>&1

    echo "[OK] vtune_results/hotspots_sw_N${N}_T${T}"
done

# =============================================================================
# Exportar relatórios em texto
# =============================================================================
echo ""
echo ">>> Exportando sumários em texto..."

for T in $THREADS_VTUNE; do
    DIR="vtune_results/hpc_N${N}_T${T}"
    OUT="vtune_results/summaries/hpc_T${T}.txt"
    vtune -report summary -result-dir "$DIR" -format text -report-output "$OUT" 2>/dev/null
    echo "[OK] $OUT"
done

for T in 1 20; do
    DIR="vtune_results/hotspots_sw_N${N}_T${T}"
    OUT="vtune_results/summaries/hotspots_T${T}.txt"
    vtune -report summary -result-dir "$DIR" -format text -report-output "$OUT" 2>/dev/null
    echo "[OK] $OUT"
done

echo ""
echo "=================================================="
echo " CONCLUIDO! $(date)"
echo " Resultados em: vtune_results/"
echo " Sumarios em:   vtune_results/summaries/"
echo "=================================================="
