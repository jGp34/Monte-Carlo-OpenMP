#!/bin/bash
#SBATCH --partition=hype
#SBATCH --job-name=mc_vtune
#SBATCH --output=saida_vtune.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=40
#SBATCH --time=03:00:00

source /home/intel/oneapi/vtune/2021.1.1/vtune-vars.sh

gcc -O2 -g -fopenmp -o monte_carlo_omp_vtune monte_carlo_omp.c -lm

N=500000000
SEED=42
THREADS_VTUNE="1 2 4 8 10 20 40"

mkdir -p new_results_for_monte_carlo

for T in $THREADS_VTUNE; do
    export OMP_NUM_THREADS=$T
    echo "--- hpc-performance | N=$N | Threads=$T ---"

    ./monte_carlo_omp_vtune $N $SEED

    # O "--" garante que o VTune saiba onde começam os argumentos do seu programa C
    vtune -collect hpc-performance -knob sampling-mode=sw  \
          -result-dir new_results_for_monte_carlo/hpc_N${N}_T${T} \
          -- ./monte_carlo_omp_vtune $N $SEED

    echo "[OK] new_results_for_monte_carlo/hpc_N${N}_T${T}"
done