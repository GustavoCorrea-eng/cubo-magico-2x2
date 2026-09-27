#include "Busca.hpp"
#include "Fronteira.hpp"
#include <algorithm>
#include <chrono>

struct No {
    Cubo cubo;
    int pai;
    uint8_t movimento;
    uint8_t profundidade;
};

struct MemoriaDaBusca {
    std::vector<No> pool;
    std::vector<uint8_t> menorProfundidade;
    std::vector<int> noDoEstado;

    MemoriaDaBusca()
        : menorProfundidade(N_ESTADOS, 0xFF), noDoEstado(N_ESTADOS, -1)
    {
        pool.reserve(1 << 16);
    }

    void reiniciar()
    {
        pool.clear();
        std::fill(menorProfundidade.begin(), menorProfundidade.end(), 0xFF);
        std::fill(noDoEstado.begin(), noDoEstado.end(), -1);
    }

    int novoNo(const Cubo &c, int pai, int movimento, int profundidade)
    {
        pool.push_back({c, pai, (uint8_t) movimento, (uint8_t) profundidade});
        return (int) pool.size() - 1;
    }

    void reconstruirCaminho(int no, Resultado &r) const
    {
        std::vector<int> invertido;
        while (no >= 0 && pool[(size_t) no].pai >= 0) {
            invertido.push_back(pool[(size_t) no].movimento);
            no = pool[(size_t) no].pai;
        }
        r.passos.assign(invertido.rbegin(), invertido.rend());
    }
};

bool ehObjetivo(const Cubo &c) { return estaResolvido(c); }

int heuristica(const Cubo &c)
{
    int fora = 0;
    for (int i = 0; i < 7; i++)
        if (c.cp[i] != i || c.co[i] != 0) fora++;
    return (fora + 3) / 4;
}

static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    mem.menorProfundidade[indiceDoEstado(inicial)] = 0;
    mem.noDoEstado[indiceDoEstado(inicial)] = raiz;
    fr.inserir(raiz, heuristica(inicial));
    r.gerados++;

    while (!fr.vazia()) {
        int atual = fr.remover();
        Cubo estado = mem.pool[(size_t) atual].cubo;
        int prof    = mem.pool[(size_t) atual].profundidade;
        r.visitados++;

        if (ehObjetivo(estado)) {
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }

        if (limite >= 0 && prof >= limite) continue;

        for (int m = 0; m < N_MOVIMENTOS; m++) {
            if (mem.pool[(size_t) atual].pai >= 0 &&
                mesmaFace(m, mem.pool[(size_t) atual].movimento))
                continue;

            Cubo vizinho = mover(estado, m);
            uint32_t idx = indiceDoEstado(vizinho);

            if (mem.menorProfundidade[idx] <= prof + 1) continue;
            mem.menorProfundidade[idx] = (uint8_t) (prof + 1);

            int filho = mem.noDoEstado[idx];
            if (filho >= 0) {
                mem.pool[(size_t) filho].pai          = atual;
                mem.pool[(size_t) filho].movimento    = (uint8_t) m;
                mem.pool[(size_t) filho].profundidade = (uint8_t) (prof + 1);
            } else {
                filho = mem.novoNo(vizinho, atual, m, prof + 1);
                mem.noDoEstado[idx] = filho;
            }
            fr.inserir(filho, (prof + 1) + heuristica(vizinho));
            r.gerados++;
        }
    }

    r.encontrou = false;
}

Resultado buscaEmLargura(const Cubo &inicial)
{
    Resultado r;
    auto t0 = std::chrono::steady_clock::now();

    MemoriaDaBusca mem;
    FilaFronteira fr;
    lacoDeBusca(fr, mem, inicial, -1, r);

    r.segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

Resultado buscaAEstrela(const Cubo &inicial)
{
    Resultado r;
    auto t0 = std::chrono::steady_clock::now();

    MemoriaDaBusca mem;
    PrioridadeFronteira fr;
    lacoDeBusca(fr, mem, inicial, -1, r);

    r.segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

Resultado buscaProfundidadeIterativa(const Cubo &inicial, int limiteMaximo)
{
    Resultado total;
    auto t0 = std::chrono::steady_clock::now();
    MemoriaDaBusca mem;

    for (int limite = 0; limite <= limiteMaximo; limite++) {
        Resultado r;
        PilhaFronteira fr;

        mem.reiniciar();
        lacoDeBusca(fr, mem, inicial, limite, r);

        total.visitados += r.visitados;
        total.gerados   += r.gerados;
        total.limiteFinal = limite;

        if (r.encontrou) {
            total.encontrou = true;
            total.passos = r.passos;
            break;
        }
    }

    total.segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return total;
}
