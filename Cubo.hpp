// ============================================================
// Cubo.hpp - ESTADO do problema e FUNCAO SUCESSORA
// Simulador de cubo magico 2x2x2
//
// Este cabecalho declara tudo o que o resto do programa precisa
// saber sobre o cubo: como o estado e representado, como aplicar
// um movimento, como identificar um estado por um numero unico,
// como embaralhar de forma reproduzivel e como desenhar o cubo.
// A implementacao de cada funcao esta em Cubo.cpp.
// ============================================================

// Garante que este arquivo so seja incluido uma vez por compilacao.
#pragma once

// std::array: vetor de tamanho fixo (usado no estado e nas tabelas).
#include <array>
// uint8_t / uint32_t: inteiros com tamanho exato em bits.
#include <cstdint>
// std::string: nomes dos movimentos ("R'", "U2"...).
#include <string>
// std::vector: lista de movimentos devolvida pelo embaralhamento.
#include <vector>

// Cada movimento gira uma das 3 faces livres (U, R, F) por 90, 180
// ou 270 graus no sentido horario. Codigo do movimento = face*3 + variacao.

// Quantidade de cantos (pecas) do cubo 2x2x2.
constexpr int N_CANTOS     = 8;
// Quantidade de movimentos possiveis: 3 faces x 3 variacoes.
constexpr int N_MOVIMENTOS = 9;              // U U2 U' R R2 R' F F2 F'
// Quantos estados diferentes o cubo pode ter (sem contar rotacoes do cubo todo).
constexpr uint32_t N_ESTADOS = 3'674'160u;   // 7! * 3^6 = espaco de estados alcancavel

// Nomes dos 9 movimentos, na mesma ordem dos codigos 0..8.
// Ex.: NOME_MOVIMENTO[5] e "R'" (giro anti-horario da face R).
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
    // Permutacao: cp[i] diz qual peca esta ocupando a posicao i.
    std::array<uint8_t, N_CANTOS> cp;
    // Orientacao: co[i] diz quantos giros de 120 graus a peca da posicao i sofreu.
    std::array<uint8_t, N_CANTOS> co;
};

// Devolve um cubo no estado resolvido (cada peca no seu lugar, orientacao 0).
Cubo cuboResolvido();
// Diz se o cubo esta resolvido. E a base da FUNCAO AVALIADORA da busca.
bool estaResolvido(const Cubo &c);

// FUNCAO SUCESSORA: devolve o estado apos aplicar um movimento (0..8).
// Nao altera o cubo original: cria e devolve um novo estado.
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
// Devolve a lista dos movimentos aplicados, na ordem.
std::vector<int> embaralhar(Cubo &c, unsigned int semente, int n);

// Converte o nome de um movimento ("U", "R2", "F'") no seu codigo 0..8.
int movimentoPorNome(const std::string &s);   // "R'" -> 5, ou -1 se invalido
// Diz se dois movimentos giram a mesma face (ex.: R e R2 giram a face R).
bool mesmaFace(int movA, int movB);
// Devolve o movimento que desfaz o dado (R vira R', R2 continua R2).
int  movimentoInverso(int mov);

// ------------------------------------------------------------
// VISUALIZACAO
//
// O estado interno (pecas + orientacao) e convertido nas 24
// "figurinhas" do cubo e impresso como uma planificacao colorida.
// Faces: 0=U(branco) 1=R(vermelho) 2=F(verde) 3=D(amarelo)
//        4=L(laranja) 5=B(azul)
// ------------------------------------------------------------
// CANTO_FACELET[i][k] = indice (0..23) da figurinha que fica no k-esimo
// adesivo visivel da posicao de canto i (ver Cubo.cpp para o desenho da
// planificacao). Exposta para quem precisar desenhar o cubo de outro jeito
// (ex.: a janela 3D em Janela3D.cpp) sem duplicar esta tabela.
extern const uint8_t CANTO_FACELET[N_CANTOS][3];

// Converte o estado (pecas + orientacao) nas 24 cores das figurinhas.
// O indice do vetor devolvido e a posicao da figurinha na planificacao.
std::array<uint8_t, 24> facelets(const Cubo &c);

// Planificacao completa (todas as 6 faces).
void imprimirCubo(const Cubo &c);

// Vista de "canto" - so as 3 faces jogaveis (U, F, R), desenhadas
// com um leve efeito de profundidade. Ver comentario em Cubo.cpp.
void imprimirCuboIso(const Cubo &c);

// Liga o suporte a cores (codigos ANSI) no terminal do Windows.
// Sem isso, o console antigo mostraria os codigos de cor como texto.
void habilitarCoresNoTerminal();     // liga ANSI no console do Windows
