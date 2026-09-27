// ============================================================
// Fronteira.hpp - A ESTRUTURA DE DADOS da busca
//
// O laco de busca (em Busca.cpp) e sempre o mesmo; o que muda
// entre Largura, Profundidade e A* e apenas QUAL estrutura
// guarda os estados ainda nao expandidos. Por isso as tres
// estruturas implementam a mesma interface abstrata, e o laco
// nunca precisa saber qual delas esta usando por baixo -
// polimorfismo no lugar dos ponteiros de funcao.
//
//     FilaFronteira       (FIFO)              -> Busca em Largura
//     PilhaFronteira      (LIFO)              -> Busca em Profundidade Limitada
//     PrioridadeFronteira (heap por f = g+h)  -> Busca A*
//
// O parametro "chave" de inserir() so importa para a fila de
// prioridade; a fila e a pilha simplesmente o ignoram.
// ============================================================

// Garante que este arquivo so seja incluido uma vez por compilacao.
#pragma once
// push_heap / pop_heap: operacoes de heap usadas pela fila de prioridade.
#include <algorithm>
// std::deque: fila com insercao no fim e remocao no inicio (usada na FIFO).
#include <deque>
#include <string>
// std::vector: armazena a pilha e o heap.
#include <vector>

// Interface comum das tres estruturas. E uma classe abstrata: nao da
// para criar um objeto dela, so das classes concretas que a implementam.
class Fronteira {
public:
    // Destrutor virtual: garante que a classe concreta seja destruida
    // corretamente mesmo quando usada por uma referencia a Fronteira.
    virtual ~Fronteira() = default;
    // Coloca um no na estrutura. "chave" e a prioridade (so o A* usa).
    virtual void inserir(int idNo, int chave) = 0;
    // Tira da estrutura o proximo no a ser expandido, na ordem propria de
    // cada estrutura (o primeiro, o ultimo ou o de menor chave).
    virtual int  remover() = 0;                 // devolve o id do no
    // Diz se nao ha mais nenhum no esperando.
    virtual bool vazia() const = 0;
    // Nome legivel da estrutura (para mensagens).
    virtual const char *nome() const = 0;
};

// 1) FILA (FIFO) -> Busca em Largura
// O primeiro que entra e o primeiro que sai: expande os estados por
// ordem de chegada, ou seja, camada por camada.
class FilaFronteira : public Fronteira {
    // Guarda os ids dos nos; deque permite tirar do inicio de forma eficiente.
    std::deque<int> d;
public:
    // Poe no fim da fila. O segundo parametro (chave) e ignorado, por isso sem nome.
    void inserir(int idNo, int) override { d.push_back(idNo); }
    // Tira do inicio da fila (o que esta esperando ha mais tempo).
    int  remover() override { int n = d.front(); d.pop_front(); return n; }
    bool vazia() const override { return d.empty(); }
    const char *nome() const override { return "Fila (FIFO)"; }
};

// 2) PILHA (LIFO) -> Busca em Profundidade Limitada
// O ultimo que entra e o primeiro que sai: segue um caminho ate o
// fim antes de voltar e tentar outro.
class PilhaFronteira : public Fronteira {
    // O fim do vetor funciona como o topo da pilha.
    std::vector<int> v;
public:
    // Empilha no topo. A chave e ignorada.
    void inserir(int idNo, int) override { v.push_back(idNo); }
    // Desempilha o que esta no topo (o mais recente).
    int  remover() override { int n = v.back(); v.pop_back(); return n; }
    bool vazia() const override { return v.empty(); }
    const char *nome() const override { return "Pilha (LIFO)"; }
};

// 3) FILA DE PRIORIDADE (heap binario de minimo) -> A*
// Ordena pela chave f = g + h. Em empate, sai primeiro o no
// inserido por ultimo (o mais profundo, que tende a estar mais
// perto da solucao).
class PrioridadeFronteira : public Fronteira {
    // Cada entrada do heap: o no, sua prioridade (f = g + h) e a ordem
    // de chegada (usada para desempatar).
    struct Item { int idNo, chave; unsigned long ordem; };
    // Regra de comparacao que o heap usa para decidir quem sai primeiro.
    struct Comparador {
        // std::priority_queue e um max-heap; invertendo a comparacao
        // ele vira um min-heap (sai primeiro quem "e menor").
        bool operator()(const Item &a, const Item &b) const
        {
            // Chaves diferentes: sai a menor chave.
            if (a.chave != b.chave) return a.chave > b.chave;
            // Chaves iguais: sai quem entrou por ultimo (ordem maior).
            return a.ordem < b.ordem;
        }
    };
    // O heap em si, guardado num vetor e mantido pelas funcoes de heap da STL.
    std::vector<Item> dados;
    // Contador que numera as insercoes (usado no desempate).
    unsigned long contador = 0;

public:
    PrioridadeFronteira() { }
    // Insere no fim do vetor e "sobe" o item ate a posicao certa do heap.
    void inserir(int idNo, int chave) override
    {
        dados.push_back({idNo, chave, contador++});
        std::push_heap(dados.begin(), dados.end(), Comparador());
    }
    // Leva o melhor item (menor chave) para o fim do vetor, le o seu id e o descarta.
    int remover() override
    {
        std::pop_heap(dados.begin(), dados.end(), Comparador());
        int n = dados.back().idNo;
        dados.pop_back();
        return n;
    }
    bool vazia() const override { return dados.empty(); }
    const char *nome() const override { return "Fila de prioridade (heap por f=g+h)"; }
};
