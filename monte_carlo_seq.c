/*
 * Monte Carlo Sequencial
 * Disciplina: Programação Paralela - UFRGS
 *
 * Implementação sequencial de dois problemas via Método de Monte Carlo:
 *   1. Estimativa de Pi (pi_estimation)
 *   2. Integração numérica de f(x) = e^(-x^2) em [0, 1]
 *      (cujo valor analítico é sqrt(pi)/2 * erf(1) ≈ 0.7468241328...)
 *
 * Uso:
 *   gcc -O2 -o monte_carlo_seq monte_carlo_seq.c -lm
 *   ./monte_carlo_seq <N> <seed>
 *     N    = número de amostras (ex: 100000000)
 *     seed = semente para o gerador de números aleatórios (ex: 42)
 *
 * Saída (CSV):
 *   tipo,N,resultado,erro_relativo,tempo_s
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Valor de referência para Pi */
#define PI_REF 3.14159265358979323846

/* Valor de referência para integral de e^(-x^2) em [0,1] */
#define INTEGRAL_REF 0.74682413281242702540

/* ------------------------------------------------------------------ */
/* Leitura de tempo de parede com clock_gettime (alta resolução)       */
/* ------------------------------------------------------------------ */
static double wall_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ------------------------------------------------------------------ */
/* 1. Estimativa de Pi                                                 */
/*    Lança N pontos aleatórios no quadrado [0,1]x[0,1] e conta       */
/*    quantos caem no interior do quarto de círculo unitário.          */
/*    Pi ≈ 4 * (pontos_dentro / N)                                     */
/* ------------------------------------------------------------------ */
double estimate_pi(long N, unsigned short seed[3])
{
    long inside = 0;

    for (long i = 0; i < N; i++) {
        double x = erand48(seed);   /* uniforme em [0,1) */
        double y = erand48(seed);
        if (x * x + y * y <= 1.0)
            inside++;
    }

    return 4.0 * (double)inside / (double)N;
}

/* ------------------------------------------------------------------ */
/* 2. Integração numérica: integral de e^(-x^2) em [0, 1]             */
/*    Método: Monte Carlo simples (amostragem uniforme em [0,1])       */
/*    Estimativa = media de f(x_i)                                     */
/* ------------------------------------------------------------------ */
double monte_carlo_integral(long N, unsigned short seed[3])
{
    double sum = 0.0;

    for (long i = 0; i < N; i++) {
        double x = erand48(seed);
        sum += exp(-x * x);
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

    /* Semente compatível com erand48 (48-bit) */
    unsigned short seed_pi[3]       = { (unsigned short)(iseed),
                                        (unsigned short)(iseed >> 16),
                                        (unsigned short)(iseed ^ 0xAAAA) };
    unsigned short seed_integ[3]    = { (unsigned short)(iseed ^ 0x1234),
                                        (unsigned short)(iseed >> 8),
                                        (unsigned short)(iseed ^ 0x5678) };

    double t0, t1, result, error;

    /* Cabeçalho CSV */
    printf("tipo,N,resultado,referencia,erro_relativo,tempo_s\n");

    /* --- Pi estimation --- */
    t0     = wall_time();
    result = estimate_pi(N, seed_pi);
    t1     = wall_time();
    error  = fabs(result - PI_REF) / PI_REF;
    printf("pi,%ld,%.10f,%.10f,%.6e,%.6f\n",
           N, result, PI_REF, error, t1 - t0);

    /* --- Integração numérica --- */
    t0     = wall_time();
    result = monte_carlo_integral(N, seed_integ);
    t1     = wall_time();
    error  = fabs(result - INTEGRAL_REF) / INTEGRAL_REF;
    printf("integral,%ld,%.10f,%.10f,%.6e,%.6f\n",
           N, result, INTEGRAL_REF, error, t1 - t0);

    return EXIT_SUCCESS;
}