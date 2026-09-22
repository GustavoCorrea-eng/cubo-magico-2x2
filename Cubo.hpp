// ============================================================
// Cubo.hpp - ESTADO do problema e FUNCAO SUCESSORA
// Simulador de cubo magico 2x2x2
// ============================================================
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Cada movimento gira uma das 3 faces livres (U, R, F) por 90, 180
// ou 270 graus no sentido horario. Codigo do movimento = face*3 + variacao.
constexpr int N_CANTOS     = 8;
constexpr int N_MOVIMENTOS = 9;              // U U2 U' R R2 R' F F2 F'
constexpr uint32_t N_ESTADOS = 3'674'160u;   // 7! * 3^6 = espaco de estados alcancavel

// Nomes dos 9 movimentos, na mesma ordem dos codigos 0..8.
extern const std::array<std::string, N_MOVIMENTOS> NOME_MOVIMENTO;

// ------------------------------------------------------------
// ESTADO
//
// Um cubo 2x2x2 tem 8 pecas de canto e nenhum centro, entao a
// orientacao do cubo inteiro (como bloco rigido) e livre. Por
// isso FIXAMOS a peca no canto DBL (indice 7) e giramos apenas
// as tres faces que nunca a tocam: U, R e F. Isso alcanca todo
// estado possivel (um giro do cubo inteiro nao muda o quebra-
// cabeca) e reduz o espaco de busca por um fator de 24.
//
//   Posicoes de canto:
//     0 = URF   1 = UFL   2 = ULB   3 = UBR
//     4 = DFR   5 = DLF   6 = DRB   7 = DBL  (fixa)
//
//   cp[i] = qual PECA esta na posicao i        (permutacao)
//   co[i] = orientacao dessa peca: 0, 1 ou 2   (giros de 120 graus)
//
//   Estado resolvido: cp[i] == i e co[i] == 0 para todo i.
// ------------------------------------------------------------
struct Cubo {
    std::array<uint8_t, N_CANTOS> cp;
    std::array<uint8_t, N_CANTOS> co;
};

Cubo cuboResolvido();
bool estaResolvido(const Cubo &c);

// FUNCAO SUCESSORA: devolve o estado apos aplicar um movimento (0..8).
Cubo mover(const Cubo &origem, int movimento);

// Indice unico e compacto do estado, de 0 a N_ESTADOS-1: codigo de
// Lehmer da permutacao combinado com a orientacao em base 3. Permite
// marcar "estado ja visitado" em O(1) com um vetor simples.
uint32_t indiceDoEstado(const Cubo &c);

// Embaralha a partir do estado resolvido usando uma semente propria
// (nao usa rand(), cujo resultado varia entre compiladores/maquinas).
// A mesma semente + o mesmo numero de movimentos sempre gera o
// mesmo cubo, em qualquer maquina - por isso da para "refazer" o
// embaralhamento e comparar as tres estrategias no mesmo caso.
std::vector<int> embaralhar(Cubo &c, unsigned int semente, int n);

int movimentoPorNome(const std::string &s);   // "R'" -> 5, ou -1 se invalido
bool mesmaFace(int movA, int movB);
int  movimentoInverso(int mov);

// ------------------------------------------------------------
// VISUALIZACAO
//
// O estado interno (pecas + orientacao) e convertido nas 24
// "figurinhas" do cubo e impresso como uma planificacao colorida.
// Faces: 0=U(branco) 1=R(vermelho) 2=F(verde) 3=D(amarelo)
//        4=L(laranja) 5=B(azul)
// ------------------------------------------------------------
std::array<uint8_t, 24> facelets(const Cubo &c);
void imprimirCubo(const Cubo &c);
void habilitarCoresNoTerminal();     // liga ANSI no console do Windows
