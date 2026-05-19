#!/bin/bash
# =============================================================================
# run_experiments.slurm
# Job Slurm para execução dos experimentos de desempenho (Monte Carlo - OpenMP)
# Partição: hype (necessária para Intel VTune)
#   hype1/2/3: 2x Intel Xeon E5-2650 v3 -> 20 cores, 40 threads (HT)
# =============================================================================

#SBATCH --job-name=mc_omp_exp
#SBATCH --partition=hype
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=40          # 40 threads lógicos disponíveis no hype
#SBATCH --time=04:00:00
#SBATCH --output=logs/mc_exp_%j.out
#SBATCH --error=logs/mc_exp_%j.err

# =============================================================================
# Configuração do ambiente
# =============================================================================
mkdir -p logs results

echo "======================================================="
echo "  Job ID     : $SLURM_JOB_ID"
echo "  Node       : $SLURMD_NODENAME"
echo "  CPUs alloc : $SLURM_CPUS_PER_TASK"
echo "  Data/Hora  : $(date)"
echo "======================================================="

# Fixar frequência de CPU no máximo para consistência dos resultados
for i in $(seq 0 $(( $(nproc) - 1 ))); do
    maxf=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/cpuinfo_max_freq 2>/dev/null || echo 0)
    if [ "$maxf" -gt 0 ]; then
        cpufreq-set -c $i -g performance -d $maxf -u $maxf 2>/dev/null
    fi
done
echo "[INFO] CPU governor: performance"

# =============================================================================
# Compilação
# =============================================================================
echo ""
echo "=== Compilando ==="

gcc -O2 -o monte_carlo_seq monte_carlo_seq.c -lm
if [ $? -ne 0 ]; then echo "[ERRO] Falha na compilação sequencial"; exit 1; fi
echo "[OK] monte_carlo_seq compilado"

gcc -O2 -fopenmp -o monte_carlo_omp monte_carlo_omp.c -lm
if [ $? -ne 0 ]; then echo "[ERRO] Falha na compilação paralela"; exit 1; fi
echo "[OK] monte_carlo_omp compilado"

# =============================================================================
# Parâmetros dos experimentos
# =============================================================================

# Tamanhos de entrada (N = número de amostras)
SIZES="10000000 50000000 100000000 500000000 1000000000"

# Número de threads a testar
THREADS="1 2 4 8 10 20 40"

# Número de repetições por configuração (para média estatística)
REPS=5

SEED=42

# Arquivo de saída agregado
OUTFILE="results/resultados_todos.csv"
echo "tipo,N,threads,resultado,referencia,erro_relativo,tempo_s,rep" > "$OUTFILE"

# =============================================================================
# Experimento 1: Versão Sequencial (baseline)
# =============================================================================
echo ""
echo "=== Executando versão SEQUENCIAL ==="

for SIZE in $SIZES; do
    echo -n "  N=$SIZE "
    for rep in $(seq 1 $REPS); do
        out=$(./monte_carlo_seq $SIZE $SEED)
        # Pula cabeçalho e adiciona coluna de repetição
        echo "$out" | tail -n +2 | while IFS= read -r line; do
            echo "${line},${rep}" >> "$OUTFILE"
        done
        echo -n "."
    done
    echo " OK"
done

# =============================================================================
# Experimento 2: Versão Paralela com OpenMP
# =============================================================================
echo ""
echo "=== Executando versão PARALELA (OpenMP) ==="

for SIZE in $SIZES; do
    echo "  N=$SIZE"
    for T in $THREADS; do
        echo -n "    threads=$T "
        for rep in $(seq 1 $REPS); do
            out=$(OMP_NUM_THREADS=$T ./monte_carlo_omp $SIZE $SEED)
            echo "$out" | tail -n +2 | while IFS= read -r line; do
                echo "${line},${rep}" >> "$OUTFILE"
            done
            echo -n "."
        done
        echo " OK"
    done
done

echo ""
echo "=== Experimentos concluídos. Resultados em: $OUTFILE ==="
echo "======================================================="
echo "Fim: $(date)"
