// ============================================================
// Busca.cpp - implementacao das tres buscas de IA (Largura,
// Profundidade Limitada Iterativa e A*), todas usando O MESMO
// laco de busca (lacoDeBusca), so trocando a estrutura de dados.
// ============================================================
#include "Busca.hpp"
#include "Fronteira.hpp"
// std::fill: usado para limpar os vetores de controle.
#include <algorithm>
// steady_clock: cronometro usado para medir o tempo de cada busca.
#include <chrono>

// ------------------------------------------------------------
// No da arvore de busca: guarda o pai e o movimento que levou
// ate aqui, para reconstruir o caminho no final.
// ------------------------------------------------------------
struct No {
    // O estado do cubo que este no representa.
    Cubo cubo;
    // Posicao (no vetor de nos) do no de onde este veio; -1 para o no inicial.
    int pai;
    // Movimento aplicado ao pai para chegar neste no.
    uint8_t movimento;
    // Quantos movimentos separam este no do estado inicial.
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
    // Todos os nos criados ate agora (o "id" de um no e a sua posicao aqui).
    std::vector<No> pool;
    std::vector<uint8_t> menorProfundidade;   // por estado: menor profundidade ja vista
    std::vector<int> noDoEstado;              // por estado: no que o representa (-1 = nenhum)

    // Aloca os dois vetores de controle com um item para cada estado possivel.
    // 0xFF (255) significa "ainda nao visto"; -1 significa "sem no ainda".
    MemoriaDaBusca()
        : menorProfundidade(N_ESTADOS, 0xFF), noDoEstado(N_ESTADOS, -1)
    {
        // Reserva espaco para varios nos de uma vez, evitando realocar toda hora.
        pool.reserve(1 << 16);
    }

    // Deixa a memoria como nova (usado entre as rodadas da profundidade iterativa).
    void reiniciar()
    {
        pool.clear();
        std::fill(menorProfundidade.begin(), menorProfundidade.end(), 0xFF);
        std::fill(noDoEstado.begin(), noDoEstado.end(), -1);
    }

    // Cria um no novo, guarda no pool e devolve o seu id.
    int novoNo(const Cubo &c, int pai, int movimento, int profundidade)
    {
        pool.push_back({c, pai, (uint8_t) movimento, (uint8_t) profundidade});
        return (int) pool.size() - 1;
    }

    // Monta a lista de movimentos da solucao seguindo os "pais" do no
    // objetivo ate o no inicial, e grava em r.passos.
    void reconstruirCaminho(int no, Resultado &r) const
    {
        // Andando do fim para o comeco, os movimentos saem de tras para frente...
        std::vector<int> invertido;
        while (no >= 0 && pool[(size_t) no].pai >= 0) {
            invertido.push_back(pool[(size_t) no].movimento);
            no = pool[(size_t) no].pai;
        }
        // ...entao a lista e copiada ao contrario para ficar na ordem certa.
        r.passos.assign(invertido.rbegin(), invertido.rend());
    }
};

// ============================================================
// FUNCAO AVALIADORA
// ============================================================
// O estado e o objetivo quando o cubo esta resolvido.
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
    // Conta os cantos moveis (0 a 6) que estao fora do lugar
    // (peca errada) ou com orientacao diferente de 0.
    int fora = 0;
    for (int i = 0; i < 7; i++)
        if (c.cp[i] != i || c.co[i] != 0) fora++;
    // (fora + 3) / 4 e o teto de fora/4 em divisao inteira.
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
// Parametros:
//   fr      - a estrutura de dados (fila, pilha ou heap) que define a estrategia
//   mem     - memoria da busca (nos e vetores de controle)
//   inicial - o cubo embaralhado de onde a busca parte
//   limite  - profundidade maxima (-1 = sem limite)
//   r       - onde o resultado e gravado
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    // 1. Adicionar estado na estrutura
    // O estado inicial vira o no raiz (sem pai, profundidade 0) e e
    // marcado como visto.
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    mem.menorProfundidade[indiceDoEstado(inicial)] = 0;
    mem.noDoEstado[indiceDoEstado(inicial)] = raiz;
    fr.inserir(raiz, heuristica(inicial));
    r.gerados++;

    // 2. Enquanto a estrutura nao estiver vazia
    while (!fr.vazia()) {
        // 2.1 Remover proximo estado da estrutura
        // Quem sai primeiro depende da estrutura (fila, pilha ou heap).
        int atual = fr.remover();
        Cubo estado = mem.pool[(size_t) atual].cubo;
        int prof    = mem.pool[(size_t) atual].profundidade;
        // Retirar da estrutura conta como "visitar" o estado.
        r.visitados++;

        // 2.2 Avaliar estado
        if (ehObjetivo(estado)) {
            // 2.2.1 SE estado final -> mostrar solucao e encerrar
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }

        // 2.3 Adicionar estados seguintes na estrutura
        // Se ja chegou no limite de profundidade, nao expande este estado.
        if (limite >= 0 && prof >= limite) continue;

        // Gera um vizinho para cada um dos 9 movimentos possiveis.
        for (int m = 0; m < N_MOVIMENTOS; m++) {
            // girar duas vezes seguidas a mesma face e sempre desperdicio
            // (R R2 e o mesmo que R', que ja e gerado por outro movimento)
            if (mem.pool[(size_t) atual].pai >= 0 &&
                mesmaFace(m, mem.pool[(size_t) atual].movimento))
                continue;

            // Aplica o movimento (funcao sucessora) e calcula o indice unico do vizinho.
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
            // Coloca o vizinho na estrutura. A chave f = g + h (custo ate aqui
            // + estimativa do que falta) so e usada pela fila de prioridade.
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
// Busca em Largura: fila FIFO, sem limite de profundidade.
Resultado buscaEmLargura(const Cubo &inicial)
{
    Resultado r;
    // Inicia o cronometro.
    auto t0 = std::chrono::steady_clock::now();

    MemoriaDaBusca mem;
    FilaFronteira fr;
    lacoDeBusca(fr, mem, inicial, -1, r);

    // Para o cronometro e grava o tempo decorrido em segundos.
    r.segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

// Busca A*: fila de prioridade (heap), sem limite de profundidade.
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
    // "total" acumula as estatisticas de todas as rodadas.
    Resultado total;
    auto t0 = std::chrono::steady_clock::now();
    MemoriaDaBusca mem;

    for (int limite = 0; limite <= limiteMaximo; limite++) {
        // Cada rodada tem o seu proprio resultado e uma pilha nova (LIFO).
        Resultado r;
        PilhaFronteira fr;

        // Limpa a memoria para a rodada comecar do zero.
        mem.reiniciar();
        lacoDeBusca(fr, mem, inicial, limite, r);

        // Soma o esforco desta rodada ao total.
        total.visitados += r.visitados;
        total.gerados   += r.gerados;
        total.limiteFinal = limite;

        // Primeira rodada que acha a solucao: guarda os passos e para.
        if (r.encontrou) {
            total.encontrou = true;
            total.passos = r.passos;
            break;
        }
    }

    total.segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return total;
}
