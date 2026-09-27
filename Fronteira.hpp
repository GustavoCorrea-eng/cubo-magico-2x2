#pragma once
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

class Fronteira {
public:
    virtual ~Fronteira() = default;
    virtual void inserir(int idNo, int chave) = 0;
    virtual int  remover() = 0;
    virtual bool vazia() const = 0;
    virtual const char *nome() const = 0;
};

class FilaFronteira : public Fronteira {
    std::deque<int> d;
public:
    void inserir(int idNo, int) override { d.push_back(idNo); }
    int  remover() override { int n = d.front(); d.pop_front(); return n; }
    bool vazia() const override { return d.empty(); }
    const char *nome() const override { return "Fila (FIFO)"; }
};

class PilhaFronteira : public Fronteira {
    std::vector<int> v;
public:
    void inserir(int idNo, int) override { v.push_back(idNo); }
    int  remover() override { int n = v.back(); v.pop_back(); return n; }
    bool vazia() const override { return v.empty(); }
    const char *nome() const override { return "Pilha (LIFO)"; }
};

class PrioridadeFronteira : public Fronteira {
    struct Item { int idNo, chave; unsigned long ordem; };
    struct Comparador {
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
