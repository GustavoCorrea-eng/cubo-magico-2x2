// ============================================================
// Busca.hpp - As tres estrategias de IA
// ============================================================
#pragma once
#include <vector>
#include "Cubo.hpp"

struct Resultado {
    bool encontrou = false;
    std::vector<int> passos;          // movimentos da solucao, na ordem
    unsigned long visitados = 0;      // estados RETIRADOS da estrutura (2.1)
    unsigned long gerados   = 0;      // estados COLOCADOS na estrutura (2.3)
    double segundos = 0.0;
    int limiteFinal = -1;             // usado pela busca em profundidade
};

// FUNCAO AVALIADORA: verdadeiro quando o estado e o objetivo.
bool ehObjetivo(const Cubo &c);

// HEURISTICA admissivel e consistente, usada pelo A*.
int heuristica(const Cubo &c);

Resultado buscaEmLargura(const Cubo &inicial);
Resultado buscaProfundidadeIterativa(const Cubo &inicial, int limiteMaximo);
Resultado buscaAEstrela(const Cubo &inicial);
