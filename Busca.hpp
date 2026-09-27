#pragma once
#include <vector>
#include "Cubo.hpp"

struct Resultado {
    bool encontrou = false;
    std::vector<int> passos;
    unsigned long visitados = 0;
    unsigned long gerados   = 0;
    double segundos = 0.0;
    int limiteFinal = -1;
};

bool ehObjetivo(const Cubo &c);

int heuristica(const Cubo &c);

Resultado buscaEmLargura(const Cubo &inicial);
Resultado buscaProfundidadeIterativa(const Cubo &inicial, int limiteMaximo);
Resultado buscaAEstrela(const Cubo &inicial);
