# tcc-kalebe-vitor
# Cluster Beowulf — Análise de Desempenho em Computação Paralela

Base experimental para análise de desempenho de algoritmos de **processamento digital de imagens** em ambientes sequenciais e paralelos utilizando **MPI (OpenMPI)** em um cluster Beowulf.

Este projeto foi desenvolvido como parte de um TCC com foco em:
- comparação entre linguagens (**C++, Java e Python**)
- execução **sequencial vs paralela**
- análise de desempenho em cluster real

---

## Objetivo do projeto

Avaliar o comportamento de diferentes linguagens em cenários de alto processamento, utilizando:

- Filtro de Mediana
- Detecção de Componentes Conexos

Executados em:
- modo sequencial
- modo paralelo com MPI

---

## O que já vem pronto

### Algoritmos implementados
- Filtro de Mediana (sequencial e paralelo)
- Detecção de Componentes Conexos (sequencial e paralelo)

### Linguagens suportadas
- C++
- Java
- Python

### Execução paralela
- Utilização de **MPI (OpenMPI)**
- Distribuição de processamento entre nós do cluster
- Testado em cluster Beowulf com 4 nós

---

## Estrutura do projeto

```text
cluster-beowulf-hpc/
├── cpp/
│   ├── sequential/
│   └── parallel/
├── java/
│   ├── sequential/
│   └── parallel/
├── python/
│   ├── sequential/
│   └── parallel/
├── images/
├── results/
├── scripts/
└── README.md
