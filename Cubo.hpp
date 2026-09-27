#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr int N_CANTOS     = 8;
constexpr int N_MOVIMENTOS = 9;
constexpr uint32_t N_ESTADOS = 3'674'160u;

extern const std::array<std::string, N_MOVIMENTOS> NOME_MOVIMENTO;

struct Cubo {
    std::array<uint8_t, N_CANTOS> cp;
    std::array<uint8_t, N_CANTOS> co;
};

Cubo cuboResolvido();
bool estaResolvido(const Cubo &c);

Cubo mover(const Cubo &origem, int movimento);

uint32_t indiceDoEstado(const Cubo &c);

std::vector<int> embaralhar(Cubo &c, unsigned int semente, int n);

int movimentoPorNome(const std::string &s);
bool mesmaFace(int movA, int movB);
int  movimentoInverso(int mov);

extern const uint8_t CANTO_FACELET[N_CANTOS][3];

std::array<uint8_t, 24> facelets(const Cubo &c);

void imprimirCubo(const Cubo &c);

void imprimirCuboIso(const Cubo &c);

void habilitarCoresNoTerminal();
