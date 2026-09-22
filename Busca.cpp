#include "Busca.hpp"
#include "Fronteira.hpp"
#include <algorithm>
#include <chrono>

// ------------------------------------------------------------
// No da arvore de busca: guarda o pai e o movimento que levou
// ate aqui, para reconstruir o caminho no final.
// ------------------------------------------------------------
struct No {
    Cubo cubo;
    int pai;
    uint8_t movimento;
    uint8_t profundidade;
};

// ------------------------------------------------------------
// Memoria de uma busca: cada estado distinto ocupa no maximo um
// no (nunca duplicamos), entao o teto de memoria e o proprio
// tamanho do espaco de estados. Fica num objeto proprio (em vez
// de variaveis globais) para que os vetores sejam liberados
// automaticamente ao final de cada chamada (RAII).
// ------------------------------------------------------------
struct MemoriaDaBusca {
    std::vector<No> pool;
    std::vector<uint8_t> menorProfundidade;   // por estado: menor profundidade ja vista
    std::vector<int> noDoEstado;              // por estado: no que o representa (-1 = nenhum)

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

// ============================================================
// FUNCAO AVALIADORA
// ============================================================
bool ehObjetivo(const Cubo &c) { return estaResolvido(c); }

// ============================================================
// HEURISTICA (admissivel e consistente)
//
// Cada movimento de face mexe em exatamente 4 dos 7 cantos
// moveis (U: posicoes 0-3; R: 0,3,4,6; F: 0,1,4,5 - nenhuma
// inclui a peca fixa DBL). Logo, um unico movimento consegue
// arrumar no maximo 4 cantos que estao fora do lugar. Se ha
// "fora" cantos errados (posicao OU orientacao), ainda faltam
// pelo menos teto(fora/4) movimentos - a heuristica nunca
// superestima o custo real, entao e ADMISSIVEL. E como o valor
// de "fora" muda no maximo 4 por movimento (logo teto(fora/4)
// muda no maximo 1), ela tambem e CONSISTENTE - o que garante
// que o A* devolve a solucao OTIMA.
// ============================================================
int heuristica(const Cubo &c)
{
    int fora = 0;
    for (int i = 0; i < 7; i++)
        if (c.cp[i] != i || c.co[i] != 0) fora++;
    return (fora + 3) / 4;
}

// ============================================================
// ================  O LACO DE BUSCA (UNICO)  =================
//
// Este laco NAO muda de uma estrategia para outra (requisito do
// enunciado). O que muda e apenas a estrutura de dados recebida
// em "fr" - trocar a estrategia e so passar outra Fronteira.
//
//   1. Adicionar estado na estrutura
//   2. Enquanto a estrutura nao estiver vazia:
//      2.1 Remover proximo estado da estrutura
//      2.2 Avaliar estado
//          2.2.1 SE estado final -> mostrar solucao e encerrar
//      2.3 Adicionar estados seguintes na estrutura
//   3. Retornar "Sem solucao"
// ============================================================
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    // 1. Adicionar estado na estrutura
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    mem.menorProfundidade[indiceDoEstado(inicial)] = 0;
    mem.noDoEstado[indiceDoEstado(inicial)] = raiz;
    fr.inserir(raiz, heuristica(inicial));
    r.gerados++;

    // 2. Enquanto a estrutura nao estiver vazia
    while (!fr.vazia()) {
        // 2.1 Remover proximo estado da estrutura
        int atual = fr.remover();
        Cubo estado = mem.pool[(size_t) atual].cubo;
        int prof    = mem.pool[(size_t) atual].profundidade;
        r.visitados++;

        // 2.2 Avaliar estado
        if (ehObjetivo(estado)) {
            // 2.2.1 SE estado final -> mostrar solucao e encerrar
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }

        // 2.3 Adicionar estados seguintes na estrutura
        if (limite >= 0 && prof >= limite) continue;

        for (int m = 0; m < N_MOVIMENTOS; m++) {
            // girar duas vezes seguidas a mesma face e sempre desperdicio
            // (R R2 e o mesmo que R', que ja e gerado por outro movimento)
            if (mem.pool[(size_t) atual].pai >= 0 &&
                mesmaFace(m, mem.pool[(size_t) atual].movimento))
                continue;

            Cubo vizinho = mover(estado, m);
            uint32_t idx = indiceDoEstado(vizinho);

            // so vale a pena reabrir um estado por um caminho mais curto
            if (mem.menorProfundidade[idx] <= prof + 1) continue;
            mem.menorProfundidade[idx] = (uint8_t) (prof + 1);

            // cada estado tem um unico no: se ja existe, so atualizamos
            // o caminho mais curto encontrado ate ele
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

    // 3. Retornar "Sem solucao"
    r.encontrou = false;
}

// ============================================================
// As tres estrategias: mesmo laco, estruturas diferentes.
// ============================================================
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

// Aprofundamento iterativo: chama o MESMO laco varias vezes,
// aumentando o limite de profundidade a cada rodada, ate achar
// uma solucao (que e, portanto, a de menor numero de movimentos).
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
