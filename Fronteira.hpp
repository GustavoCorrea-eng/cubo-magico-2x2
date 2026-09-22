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
#pragma once
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

class Fronteira {
public:
    virtual ~Fronteira() = default;
    virtual void inserir(int idNo, int chave) = 0;
    virtual int  remover() = 0;                 // devolve o id do no
    virtual bool vazia() const = 0;
    virtual const char *nome() const = 0;
};

// 1) FILA (FIFO) -> Busca em Largura
class FilaFronteira : public Fronteira {
    std::deque<int> d;
public:
    void inserir(int idNo, int) override { d.push_back(idNo); }
    int  remover() override { int n = d.front(); d.pop_front(); return n; }
    bool vazia() const override { return d.empty(); }
    const char *nome() const override { return "Fila (FIFO)"; }
};

// 2) PILHA (LIFO) -> Busca em Profundidade Limitada
class PilhaFronteira : public Fronteira {
    std::vector<int> v;
public:
    void inserir(int idNo, int) override { v.push_back(idNo); }
    int  remover() override { int n = v.back(); v.pop_back(); return n; }
    bool vazia() const override { return v.empty(); }
    const char *nome() const override { return "Pilha (LIFO)"; }
};

// 3) FILA DE PRIORIDADE (heap binario de minimo) -> A*
// Ordena pela chave f = g + h. Em empate, sai primeiro o no
// inserido por ultimo (o mais profundo, que tende a estar mais
// perto da solucao).
class PrioridadeFronteira : public Fronteira {
    struct Item { int idNo, chave; unsigned long ordem; };
    struct Comparador {
        // std::priority_queue e um max-heap; invertendo a comparacao
        // ele vira um min-heap (sai primeiro quem "e menor").
        bool operator()(const Item &a, const Item &b) const
        {
            if (a.chave != b.chave) return a.chave > b.chave;
            return a.ordem < b.ordem;
        }
    };
    std::vector<Item> dados;
    unsigned long contador = 0;

public:
    PrioridadeFronteira() { }
    void inserir(int idNo, int chave) override
    {
        dados.push_back({idNo, chave, contador++});
        std::push_heap(dados.begin(), dados.end(), Comparador());
    }
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
