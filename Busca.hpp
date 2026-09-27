// ============================================================
// Busca.hpp - As tres estrategias de IA
//
// Declara o que o resto do programa (main.cpp e Janela3D.cpp) pode
// usar da parte de busca: o formato do resultado, a funcao
// avaliadora, a heuristica e as tres funcoes que resolvem o cubo.
// ============================================================

// Garante que este arquivo so seja incluido uma vez por compilacao.
#pragma once
#include <vector>
// A busca trabalha sobre o tipo Cubo (o estado do problema).
#include "Cubo.hpp"

// Tudo o que uma busca devolve: se achou, a solucao e as estatisticas.
struct Resultado {
    // Verdadeiro se a busca chegou ao cubo resolvido.
    bool encontrou = false;
    std::vector<int> passos;          // movimentos da solucao, na ordem
    unsigned long visitados = 0;      // estados RETIRADOS da estrutura (2.1)
    unsigned long gerados   = 0;      // estados COLOCADOS na estrutura (2.3)
    // Tempo gasto pela busca, em segundos.
    double segundos = 0.0;
    int limiteFinal = -1;             // usado pela busca em profundidade
};

// FUNCAO AVALIADORA: verdadeiro quando o estado e o objetivo.
bool ehObjetivo(const Cubo &c);

// HEURISTICA admissivel e consistente, usada pelo A*.
// Estima quantos movimentos ainda faltam para resolver o cubo.
int heuristica(const Cubo &c);

// Busca em Largura: usa uma fila; acha a solucao com menos movimentos.
Resultado buscaEmLargura(const Cubo &inicial);
// Busca em Profundidade Limitada Iterativa: repete a busca aumentando o
// limite de profundidade (de 0 ate limiteMaximo) ate achar a solucao.
Resultado buscaProfundidadeIterativa(const Cubo &inicial, int limiteMaximo);
// Busca A*: usa uma fila de prioridade guiada pela heuristica.
Resultado buscaAEstrela(const Cubo &inicial);
