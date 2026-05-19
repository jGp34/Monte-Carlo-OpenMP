# Monte Carlo Paralelo com OpenMP
**Disciplina:** Programação Paralela — Instituto de Informática / UFRGS  
**Algoritmo:** Método de Monte Carlo (estimativa de π e integração numérica)  
**Paralelização:** OpenMP  
**Cluster:** PCAD/LPPD — partição `hype`

---

## 1. Estrutura do Projeto

```
monte_carlo/
├── monte_carlo_seq.c       # Implementação sequencial
├── monte_carlo_omp.c       # Implementação paralela (OpenMP)
├── run_experiments.slurm   # Job Slurm: roda todos os experimentos
├── run_vtune.slurm         # Job Slurm: análise Intel VTune
├── plot_results.py         # Geração dos gráficos (Python)
└── README.md               # Este arquivo
```

Diretórios criados automaticamente durante a execução:
```
├── logs/                   # Saída stdout/stderr dos jobs
├── results/                # CSV com todos os tempos medidos
├── plots/                  # Gráficos gerados pelo Python
└── vtune_results/          # Perfis coletados pelo VTune
```

---

## 2. Descrição do Algoritmo

### 2.1 Estimativa de π
Lança N pontos aleatórios no quadrado [0,1]×[0,1]. A fração dos pontos que caem
dentro do quarto de círculo unitário converge para π/4:

```
π ≈ 4 × (pontos com x² + y² ≤ 1) / N
```

O erro estatístico segue O(1/√N) — lei dos grandes números.

### 2.2 Integração numérica
Calcula ∫₀¹ e^(−x²) dx ≈ 0.74682413... usando amostragem uniforme:

```
∫₀¹ f(x) dx ≈ (1/N) × Σ f(xᵢ),  xᵢ ~ Uniforme[0,1]
```

### 2.3 Decisões de paralelização
| Decisão | Alternativa considerada | Escolha | Justificativa |
|---|---|---|---|
| RNG por thread | `rand()` global | `erand48()` com seed privada | `rand()` usa estado global — condição de corrida e serialização |
| Acumulação | `#pragma omp atomic` no loop | `reduction(+:var)` | `atomic` no loop interno seria gargalo; `reduction` é O(threads), não O(N) |
| Escalonamento | `dynamic` | `static` | Custo por iteração é uniforme → `static` evita overhead de escalonamento |

---

## 3. Conectando ao PCAD via SSH

```bash
ssh SEU_LOGIN@gppd-hpc.inf.ufrgs.br
```

Copiar arquivos do seu computador para o PCAD:
```bash
rsync -avP monte_carlo/ SEU_LOGIN@gppd-hpc.inf.ufrgs.br:~/monte_carlo/
```

Copiar resultados de volta para sua máquina:
```bash
rsync -avP SEU_LOGIN@gppd-hpc.inf.ufrgs.br:~/monte_carlo/results/ ./results/
rsync -avP SEU_LOGIN@gppd-hpc.inf.ufrgs.br:~/monte_carlo/vtune_results/ ./vtune_results/
```

---

## 4. Executando no Cluster

### 4.1 Submeter experimentos de desempenho
```bash
# No front-end do PCAD:
cd ~/monte_carlo
mkdir -p logs results vtune_results plots

sbatch run_experiments.slurm
```

Acompanhar o status:
```bash
squeue -u $USER        # ver jobs na fila
squeue -j JOB_ID       # ver job específico
tail -f logs/mc_exp_JOB_ID.out   # acompanhar saída em tempo real
```

### 4.2 Submeter análise VTune (após os experimentos)
```bash
sbatch run_vtune.slurm
```

### 4.3 Gerar gráficos (pode ser feito localmente)
```bash
# Instalar dependências (se necessário):
pip install pandas matplotlib numpy

# Gerar gráficos:
python3 plot_results.py results/resultados_todos.csv
```

Os gráficos serão salvos em `plots/`.

---

## 5. Compilação Manual (para testes)

```bash
# Sequencial
gcc -O2 -o monte_carlo_seq monte_carlo_seq.c -lm

# Paralelo
gcc -O2 -fopenmp -o monte_carlo_omp monte_carlo_omp.c -lm

# Paralelo com debug (para VTune)
gcc -O2 -g -fopenmp -o monte_carlo_omp_vtune monte_carlo_omp.c -lm
```

Execução rápida para teste:
```bash
./monte_carlo_seq 10000000 42
OMP_NUM_THREADS=4 ./monte_carlo_omp 10000000 42
```

---

## 6. Uso do Intel VTune (linha de comando)

Carregar o ambiente:
```bash
source /home/intel/oneapi/vtune/2021.1.1/vtune-vars.sh
```

Análises disponíveis usadas neste trabalho:

| Análise | Comando | O que mede |
|---|---|---|
| performance-snapshot | `-collect performance-snapshot` | Visão geral: CPU/memory/threading |
| hotspots | `-collect hotspots -knob sampling-mode=hw` | Funções que mais consomem CPU |
| hpc-performance | `-collect hpc-performance` | CPI, cache, bandwidth, vetorização |

Ver resultados em texto:
```bash
vtune -report summary -result-dir vtune_results/hotspots_t20 \
      -format text -report-output relatorio.txt
cat relatorio.txt
```

---

## 7. Métricas esperadas

Com N = 10⁹ e 40 threads no hype (20 cores físicos + HT):

| Threads | Speedup esperado | Eficiência esperada |
|---|---|---|
| 1 | 1.0× | 100% |
| 2 | ~2.0× | ~100% |
| 4 | ~3.9× | ~98% |
| 8 | ~7.5× | ~94% |
| 20 | ~18× | ~90% |
| 40 | ~22–28× | ~55–70% |

> **Nota sobre hyperthreading:** com 40 threads em 20 núcleos físicos,
> o speedup satura pois dois threads competem pelo mesmo núcleo de execução.
> Isso é esperado e deve ser discutido no relatório.

---

## 8. Referência ao PCAD no relatório

> Alguns experimentos deste trabalho utilizaram os recursos da infraestrutura
> PCAD, http://gppd-hpc.inf.ufrgs.br, no INF/UFRGS.