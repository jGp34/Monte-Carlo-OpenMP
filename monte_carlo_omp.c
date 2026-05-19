/*
 * Monte Carlo Paralelo com OpenMP
 * Disciplina: Programação Paralela - UFRGS
 *
 * Paraleliza as mesmas funções do monte_carlo_seq.c usando OpenMP.
 *
 * DECISÕES DE PARALELIZAÇÃO:
 * -------------------------------------------------------------------
 * 1. Geração de números aleatórios:
 *    - rand() / RAND_MAX NÃO é thread-safe (estado global).
 *    - Utilizamos erand48(seed) com sementes PRIVADAS por thread,
 *      derivadas da semente global + ID da thread. Isso garante:
 *        a) Ausência de condição de corrida no RNG.
 *        b) Reprodutibilidade (dado o mesmo N e seed).
 *        c) Distribuição estatisticamente independente entre threads.
 *
 * 2. Acumulação de resultados:
 *    - Usamos a cláusula reduction(+:var) do OpenMP.
 *    - Cada thread mantém sua própria cópia local da variável e
 *      ao final do bloco paralelo o runtime faz a soma atomicamente.
 *    - Isso evita o uso de #pragma omp atomic / critical no loop
 *      interno, que seria um gargalo grave de desempenho.
 *
 * 3. Escalonamento do loop:
 *    - schedule(static): cada thread recebe um bloco contíguo de
 *      iterações. Adequado porque o custo por iteração é uniforme.
 *
 * Uso:
 *   gcc -O2 -fopenmp -o monte_carlo_omp monte_carlo_omp.c -lm
 *   OMP_NUM_THREADS=8 ./monte_carlo_omp <N> <seed>
 *
 * Saída (CSV):
 *   tipo,N,threads,resultado,erro_relativo,tempo_s
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define PI_REF       3.14159265358979323846
#define INTEGRAL_REF 0.74682413281242702540

/* ------------------------------------------------------------------ */
/* Leitura de tempo de parede                                          */
/* ------------------------------------------------------------------ */
static double wall_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ------------------------------------------------------------------ */
/* Função auxiliar: inicializa semente erand48 por thread             */
/* ------------------------------------------------------------------ */
static void init_seed(unsigned short seed[3], long global_seed, int tid)
{
    /*
     * Cada thread recebe uma semente diferente derivada da semente
     * global e do seu ID. Usamos XOR e deslocamentos para garantir
     * que seeds próximas (tid=0,1,2,...) gerem sequências distintas.
     */
    seed[0] = (unsigned short)((global_seed ^ (tid * 0xDEAD)) & 0xFFFF);
    seed[1] = (unsigned short)((global_seed >> 16 ^ (tid * 0xBEEF)) & 0xFFFF);
    seed[2] = (unsigned short)((global_seed ^ 0xCAFE ^ tid) & 0xFFFF);
}

/* ------------------------------------------------------------------ */
/* 1. Estimativa de Pi (paralelo)                                      */
/* ------------------------------------------------------------------ */
double estimate_pi_omp(long N, long global_seed)
{
    long inside = 0;   /* variável que será reduzida */

    #pragma omp parallel reduction(+:inside)
    {
        int tid = omp_get_thread_num();

        /* Semente privada por thread */
        unsigned short seed[3];
        init_seed(seed, global_seed, tid);

        #pragma omp for schedule(static)
        for (long i = 0; i < N; i++) {
            double x = erand48(seed);
            double y = erand48(seed);
            if (x * x + y * y <= 1.0)
                inside++;
        }
    } /* fim do bloco paralelo: reduction soma 'inside' de cada thread */

    return 4.0 * (double)inside / (double)N;
}

/* ------------------------------------------------------------------ */
/* 2. Integração numérica paralela: integral de e^(-x^2) em [0, 1]   */
/* ------------------------------------------------------------------ */
double monte_carlo_integral_omp(long N, long global_seed)
{
    double sum = 0.0;

    #pragma omp parallel reduction(+:sum)
    {
        int tid = omp_get_thread_num();

        unsigned short seed[3];
        init_seed(seed, global_seed, tid);

        #pragma omp for schedule(static)
        for (long i = 0; i < N; i++) {
            double x = erand48(seed);
            sum += exp(-x * x);
        }
    }

    return sum / (double)N;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <seed>\n", argv[0]);
        fprintf(stderr, "  N    = numero de amostras\n");
        fprintf(stderr, "  seed = semente aleatoria\n");
        return EXIT_FAILURE;
    }

    long N     = atol(argv[1]);
    long iseed = atol(argv[2]);

    if (N <= 0) {
        fprintf(stderr, "Erro: N deve ser positivo.\n");
        return EXIT_FAILURE;
    }

    int nthreads = omp_get_max_threads();

    double t0, t1, result, error;

    /* Cabeçalho CSV */
    printf("tipo,N,threads,resultado,referencia,erro_relativo,tempo_s\n");

    /* --- Pi estimation --- */
    t0     = wall_time();
    result = estimate_pi_omp(N, iseed);
    t1     = wall_time();
    error  = fabs(result - PI_REF) / PI_REF;
    printf("pi,%ld,%d,%.10f,%.10f,%.6e,%.6f\n",
           N, nthreads, result, PI_REF, error, t1 - t0);

    /* --- Integração numérica --- */
    t0     = wall_time();
    result = monte_carlo_integral_omp(N, iseed ^ 0xABCDEF);
    t1     = wall_time();
    error  = fabs(result - INTEGRAL_REF) / INTEGRAL_REF;
    printf("integral,%ld,%d,%.10f,%.10f,%.6e,%.6f\n",
           N, nthreads, result, INTEGRAL_REF, error, t1 - t0);

    return EXIT_SUCCESS;
}