# TCC - Análise de Desempenho em Processamento de Imagens com Cluster Beowulf

**Trabalho de Conclusão de Curso (TCC)**
Análise comparativa de algoritmos de processamento de imagens em execução sequencial e paralela (MPI) utilizando um cluster Beowulf.

**Linguagens:** C++, Java, Python  
**Framework MPI:** OpenMPI (C++), MPJ Express (Java), mpi4py (Python)  
**Algoritmos:** Filtro de Mediana, Detecção de Componentes Conexos  
**Formato de imagem:** PGM (Portable Gray Map) P2 ASCII

---

## Estrutura do Projeto

```
tcc-kalebe-vitor/
├── cpp/
│   ├── sequential/
│   │   ├── median_filter.cpp          # Filtro de mediana sequencial
│   │   └── connected_components.cpp   # Componentes conexos sequencial
│   ├── parallel/
│   │   ├── median_filter_mpi.cpp      # Filtro de mediana paralelo (MPI)
│   │   └── connected_components_mpi.cpp  # Componentes conexos paralelo (MPI)
│   └── Makefile
├── java/
│   ├── sequential/
│   │   ├── MedianFilter.java          # Filtro de mediana sequencial
│   │   └── ConnectedComponents.java   # Componentes conexos sequencial
│   ├── parallel/
│   │   ├── MedianFilterMPI.java       # Filtro de mediana paralelo (MPJ)
│   │   └── ConnectedComponentsMPI.java  # Componentes conexos paralelo (MPJ)
│   └── Makefile
├── python/
│   ├── sequential/
│   │   ├── median_filter.py           # Filtro de mediana sequencial
│   │   └── connected_components.py    # Componentes conexos sequencial
│   ├── parallel/
│   │   ├── median_filter_mpi.py       # Filtro de mediana paralelo (mpi4py)
│   │   └── connected_components_mpi.py  # Componentes conexos paralelo (mpi4py)
│   └── requirements.txt
├── scripts/
│   ├── generate_test_image.py         # Gerador de imagens de teste
│   ├── run_benchmarks.sh              # Executa todos os benchmarks
│   ├── run_cpp.sh                     # Benchmarks C++
│   ├── run_java.sh                    # Benchmarks Java
│   ├── run_python.sh                  # Benchmarks Python
│   └── setup_cluster.sh              # Configuração do cluster
├── images/                            # Imagens de teste (geradas)
└── results/                           # Resultados dos benchmarks
```

---

## Pré-requisitos

### C++
- GCC/G++ com suporte a C++17
- OpenMPI: `sudo apt-get install openmpi-bin libopenmpi-dev`

### Java
- JDK 11+: `sudo apt-get install default-jdk`
- [MPJ Express](https://mpjexpress.org/) para execução paralela

### Python
- Python 3.8+
- mpi4py, numpy, Pillow:
  ```bash
  pip install -r python/requirements.txt
  ```

---

## Como Compilar

### C++
```bash
cd cpp
make all        # Compila sequencial e paralelo
make sequential # Apenas sequencial
make parallel   # Apenas paralelo (requer OpenMPI)
make clean      # Remove binários
```

### Java (Sequencial)
```bash
cd java
make sequential
```

### Java (Paralelo com MPJ Express)
```bash
export MPJ_HOME=/caminho/para/mpj
cd java
make parallel
```

---

## Gerar Imagens de Teste

```bash
cd scripts

# Gerar todas as imagens (256x256, 512x512, 1024x1024)
python3 generate_test_image.py

# Gerar apenas tamanho específico
python3 generate_test_image.py 512

# Gerar apenas imagens binárias de tamanho 1024
python3 generate_test_image.py 1024 binary

# Tipos disponíveis: noise (para filtro de mediana), binary (para componentes conexos)
```

As imagens são salvas em `images/`:
- `noise_NxN.pgm` — ruído gaussiano para testar o filtro de mediana
- `binary_NxN.pgm` — formas geométricas para testar componentes conexos

---

## Como Executar

### C++ Sequencial

```bash
# Filtro de Mediana
./cpp/sequential/median_filter images/noise_512x512.pgm results/output_median.pgm

# Componentes Conexos
./cpp/sequential/connected_components images/binary_512x512.pgm results/output_cc.pgm
```

### C++ Paralelo (MPI)

```bash
# Filtro de Mediana (4 processos)
mpirun -np 4 ./cpp/parallel/median_filter_mpi images/noise_512x512.pgm results/output_median_mpi.pgm

# Componentes Conexos (4 processos)
mpirun -np 4 ./cpp/parallel/connected_components_mpi images/binary_512x512.pgm results/output_cc_mpi.pgm
```

### Java Sequencial

```bash
cd java
java -cp . sequential.MedianFilter ../images/noise_512x512.pgm ../results/java_median.pgm
java -cp . sequential.ConnectedComponents ../images/binary_512x512.pgm ../results/java_cc.pgm
```

### Java Paralelo (MPJ Express)

```bash
export MPJ_HOME=/opt/mpj
$MPJ_HOME/bin/mpjrun.sh -np 4 -cp java parallel.MedianFilterMPI \
    images/noise_512x512.pgm results/java_median_mpi.pgm
$MPJ_HOME/bin/mpjrun.sh -np 4 -cp java parallel.ConnectedComponentsMPI \
    images/binary_512x512.pgm results/java_cc_mpi.pgm
```

### Python Sequencial

```bash
python3 python/sequential/median_filter.py images/noise_512x512.pgm results/py_median.pgm
python3 python/sequential/connected_components.py images/binary_512x512.pgm results/py_cc.pgm
```

### Python Paralelo (mpi4py)

```bash
mpirun -np 4 python3 python/parallel/median_filter_mpi.py \
    images/noise_512x512.pgm results/py_median_mpi.pgm
mpirun -np 4 python3 python/parallel/connected_components_mpi.py \
    images/binary_512x512.pgm results/py_cc_mpi.pgm
```

---

## Executar Todos os Benchmarks

```bash
cd scripts

# Benchmark completo (C++, Java, Python)
NP=4 bash run_benchmarks.sh

# Apenas C++
NP=4 bash run_cpp.sh

# Apenas Python
NP=4 bash run_python.sh

# Apenas Java
MPJ_HOME=/opt/mpj NP=4 bash run_java.sh
```

Os resultados são salvos em `results/benchmark_YYYYMMDD_HHMMSS.txt`.

---

## Configuração do Cluster Beowulf

```bash
# No nó master
NODES="node1 node2 node3" bash scripts/setup_cluster.sh master

# Em cada nó worker
bash scripts/setup_cluster.sh node

# Apenas máquina local (sem cluster)
bash scripts/setup_cluster.sh local
```

### Executar no Cluster

```bash
# Criar hostfile
echo "master slots=4" > hostfile
echo "node1 slots=4" >> hostfile
echo "node2 slots=4" >> hostfile

# Executar com hostfile
mpirun --hostfile hostfile -np 12 ./cpp/parallel/median_filter_mpi \
    images/noise_1024x1024.pgm results/cluster_median.pgm
```

---

## Formato dos Resultados

Cada execução imprime o tempo no formato:
```
Image loaded: 512x512
Median filter (sequential) time: 142.50 ms
Output written to results/output.pgm
```

Para execuções paralelas:
```
Image loaded: 512x512
Median filter (parallel, 4 procs) time: 38.12 ms
Output written to results/output_mpi.pgm
```

---

## Algoritmos Implementados

### Filtro de Mediana (Median Filter)
Filtro não-linear que substitui cada pixel pela mediana dos pixels em sua vizinhança 3×3. Eficaz para remoção de ruído *salt-and-pepper*. A paralelização é feita dividindo as linhas da imagem entre os processos MPI (com linhas de halo para bordas).

### Detecção de Componentes Conexos (Connected Components)
Algoritmo BFS que identifica e rotula regiões conectadas de pixels em imagens binárias (foreground: pixels > 127). A paralelização distribui as linhas entre processos, com troca de bordas para resolução de componentes que cruzam partições usando Union-Find.

---

## Notas

- O formato PGM P2 (ASCII) é usado para portabilidade e simplicidade de I/O sem dependências externas.
- Para imagens grandes (≥ 1024×1024), recomenda-se usar C++ para melhor desempenho.
- Os scripts de benchmark aceitam a variável de ambiente `NP` para controlar o número de processos MPI (padrão: 4).
- O Python puro (sem numpy) é significativamente mais lento; os resultados refletem essa diferença de linguagem.
