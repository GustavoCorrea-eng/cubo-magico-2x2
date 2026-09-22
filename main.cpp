// ============================================================
// main.cpp - Cubo magico 2x2x2: interface de texto colorida
//
// O jogador pode girar o cubo com uma unica tecla (sem apertar
// Enter), ou pedir para uma das tres IAs resolver e mostrar o
// caminho encontrado.
// ============================================================
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>
#include "Busca.hpp"
#include "Cubo.hpp"
#include "Teclado.hpp"
#ifdef COM_JANELA_3D
#include "Janela3D.hpp"
#endif

static const unsigned int SEMENTE_PADRAO       = 2024;
static const int          TAMANHO_PADRAO       = 9;
static const char        *MENSAGEM_INICIAL     = "Aperte 'e' para embaralhar, ou jogue com u r f.";

static Cubo cubo = cuboResolvido();
static std::vector<int> historico;
static unsigned int semente = SEMENTE_PADRAO;
static int tamanhoEmbaralho = TAMANHO_PADRAO;
static std::string mensagem = MENSAGEM_INICIAL;

static bool temSolucao = false;
static Resultado ultimaBusca;
static bool modoIsometrico = false;   // false = planificacao (6 faces), true = vista de canto (3 faces)

static void limparTela() { std::printf("\x1b[2J\x1b[H"); }

// ------------------------------------------------------------
// Mostra cada movimento como as teclas que voce apertaria para
// fazer o mesmo giro na mao: minuscula = horario, maiuscula =
// anti-horario, e um giro de 180 graus vira a mesma tecla
// repetida duas vezes (porque e exatamente isso: dois giros de
// 90 graus seguidos). Assim a solucao mostrada na tela e a
// solucao "digitavel" - nao precisa traduzir notacao nenhuma.
static std::string teclasDoMovimento(int mov)
{
    static const char LETRA[3] = {'u', 'r', 'f'};
    int face = mov / 3, variacao = mov % 3;
    char horario = LETRA[face];
    char antiHorario = (char) toupper(horario);

    if (variacao == 0) return std::string(1, horario);
    if (variacao == 2) return std::string(1, antiHorario);
    return std::string(1, horario) + " " + std::string(1, horario);   // 180 graus
}

static std::string nomesDosPassos(const std::vector<int> &passos)
{
    std::string s;
    for (size_t i = 0; i < passos.size(); i++) {
        if (i) s += "  ";
        s += teclasDoMovimento(passos[i]);
    }
    return s.empty() ? "(nenhum - ja estava resolvido)" : s;
}

static void desenharTela()
{
    limparTela();
    std::printf("  =============== CUBO MAGICO 2x2x2 ===============\n");
    if (modoIsometrico) imprimirCuboIso(cubo);
    else                imprimirCubo(cubo);

    std::printf("   Movimentos feitos ... %zu\n", historico.size());
    std::printf("   Semente .............. %u  (%d movimentos ao embaralhar)\n",
                semente, tamanhoEmbaralho);
    std::printf("   Situacao ............. %s\n\n",
                estaResolvido(cubo) ? "RESOLVIDO" : "embaralhado");

    if (temSolucao) {
        if (ultimaBusca.encontrou) {
            std::printf("   Ultima busca: %zu movimento(s), %lu estados visitados, %.3fs\n",
                        ultimaBusca.passos.size(), ultimaBusca.visitados, ultimaBusca.segundos);
            std::printf("   Solucao (teclas, na ordem): %s\n\n", nomesDosPassos(ultimaBusca.passos).c_str());
        } else {
            std::printf("   Ultima busca: sem solucao dentro do limite testado.\n\n");
        }
    }

    std::printf("   ---------------------------------------------------\n");
    std::printf("    JOGAR    u r f   giro horario         z   desfazer\n");
    std::printf("             U R F   giro anti-horario\n");
    std::printf("    IA       1  Busca em Largura\n");
    std::printf("             2  Profundidade Limitada Iterativa\n");
    std::printf("             3  A*\n");
    std::printf("             m  rodar as 3 e comparar numa tabela\n");
    std::printf("    OUTROS   e  embaralhar   c  cubo volta ao resolvido\n");
    std::printf("             C  reiniciar o programa (tudo)   a  aplicar solucao   q  sair\n");
    std::printf("             v  alternar vista (planificacao / canto 3D)\n");
#ifdef COM_JANELA_3D
    std::printf("             j  abrir janela 3D de verdade (raylib)\n");
#endif
    std::printf("   ---------------------------------------------------\n");
    std::printf("   >> %s\n", mensagem.c_str());
}

// ------------------------------------------------------------
static void jogar(int mov)
{
    cubo = mover(cubo, mov);
    historico.push_back(mov);
    temSolucao = false;
    mensagem = "Movimento '" + teclasDoMovimento(mov) + "' aplicado.";
}

static void desfazer()
{
    if (historico.empty()) { mensagem = "Nada para desfazer."; return; }
    int ultimo = historico.back();
    historico.pop_back();
    cubo = mover(cubo, movimentoInverso(ultimo));
    temSolucao = false;
    mensagem = "Ultimo movimento desfeito.";
}

// Reset completo: volta tudo ao estado de quando o programa acabou de abrir
// (cubo resolvido, sem historico, sem busca guardada, semente/tamanho de
// embaralhamento padrao). Diferente da tecla 'c', que so resolve o cubo.
static void resetarPrograma()
{
    cubo = cuboResolvido();
    historico.clear();
    temSolucao = false;
    ultimaBusca = Resultado();
    semente = SEMENTE_PADRAO;
    tamanhoEmbaralho = TAMANHO_PADRAO;
    modoIsometrico = false;
    mensagem = "Programa reiniciado.";
}

static void embaralharAgora()
{
    std::cout << "\n   Digite a semente (numero inteiro): ";
    std::cout.flush();
    unsigned int novaSemente;
    if (std::cin >> novaSemente) semente = novaSemente;

    std::cout << "   Quantos movimentos (1-40)? ";
    std::cout.flush();
    int novoTamanho;
    if (std::cin >> novoTamanho && novoTamanho >= 1 && novoTamanho <= 40)
        tamanhoEmbaralho = novoTamanho;

    embaralhar(cubo, semente, tamanhoEmbaralho);
    historico.clear();
    temSolucao = false;
    mensagem = "Embaralhado com a semente " + std::to_string(semente) + ".";
}

static void resolver(int estrategia)
{
    static const char *nomes[3] = {
        "Busca em Largura", "Profundidade Limitada Iterativa", "A*"
    };

    if (estaResolvido(cubo)) {
        mensagem = "O cubo ja esta resolvido.";
        return;
    }

    std::printf("\n   Rodando %s...\n", nomes[estrategia]);
    if (estrategia == 0)
        std::printf("   (a busca em largura visita muitos estados - pode levar alguns segundos)\n");
    std::fflush(stdout);

    if      (estrategia == 0) ultimaBusca = buscaEmLargura(cubo);
    else if (estrategia == 1) ultimaBusca = buscaProfundidadeIterativa(cubo, 11);
    else                      ultimaBusca = buscaAEstrela(cubo);

    temSolucao = true;
    mensagem = ultimaBusca.encontrou
                   ? std::string(nomes[estrategia]) + " encontrou a solucao. Tecle 'a' para aplicar."
                   : std::string(nomes[estrategia]) + " nao encontrou solucao.";
}

// Roda as tres estrategias no cubo atual e mostra uma tabela comparando
// estados visitados, estados gerados, tamanho da solucao e tempo.
static void compararTodas()
{
    static const char *nomes[3] = {
        "Busca em Largura", "Prof. Iterativa", "A*"
    };

    if (estaResolvido(cubo)) {
        mensagem = "O cubo ja esta resolvido.";
        return;
    }

    limparTela();
    std::printf("\n   Rodando as tres buscas no mesmo cubo (semente %u, %d movimentos)...\n",
                semente, tamanhoEmbaralho);
    std::printf("   Isso pode levar alguns segundos, principalmente a Busca em Largura.\n\n");
    std::fflush(stdout);

    Resultado r[3];
    for (int i = 0; i < 3; i++) {
        std::printf("   [%d/3] %s ... ", i + 1, nomes[i]);
        std::fflush(stdout);
        if      (i == 0) r[i] = buscaEmLargura(cubo);
        else if (i == 1) r[i] = buscaProfundidadeIterativa(cubo, 11);
        else             r[i] = buscaAEstrela(cubo);
        std::printf("pronto (%.3fs)\n", r[i].segundos);
        std::fflush(stdout);
    }

    std::printf("\n   ---------------------------------------------------------------\n");
    std::printf("   %-18s %10s %12s %12s %10s\n",
                "Estrategia", "Otimo", "Visitados", "Gerados", "Tempo(s)");
    std::printf("   ---------------------------------------------------------------\n");
    for (int i = 0; i < 3; i++) {
        if (r[i].encontrou)
            std::printf("   %-18s %10zu %12lu %12lu %10.3f\n",
                        nomes[i], r[i].passos.size(), r[i].visitados, r[i].gerados, r[i].segundos);
        else
            std::printf("   %-18s %10s %12s %12s %10.3f\n",
                        nomes[i], "-", "sem solucao", "-", r[i].segundos);
    }
    std::printf("   ---------------------------------------------------------------\n");

    if (r[0].encontrou && r[2].encontrou && r[2].visitados > 0) {
        std::printf("\n   A* visitou %.1fx menos estados que a Busca em Largura\n",
                    (double) r[0].visitados / (double) r[2].visitados);
        std::printf("   A* visitou %.1fx menos estados que a Prof. Iterativa\n",
                    (double) r[1].visitados / (double) r[2].visitados);
    }

    std::printf("\n   Aperte qualquer tecla para voltar ao menu...\n");
    std::fflush(stdout);
    lerTecla();

    // guarda o resultado do A* como a "ultima busca", pronta para aplicar com 'a'
    if (r[2].encontrou) { ultimaBusca = r[2]; temSolucao = true; }
    mensagem = "Comparacao concluida. As tres acharam solucoes de mesmo tamanho? Veja a tabela.";
}

static void aplicarSolucao()
{
    if (!temSolucao || !ultimaBusca.encontrou) {
        mensagem = "Rode uma busca antes (teclas 1, 2 ou 3).";
        return;
    }

    for (int mov : ultimaBusca.passos) {
        cubo = mover(cubo, mov);
        historico.push_back(mov);
        desenharTela();
        std::printf("\n   Aplicando '%s' (notacao classica: %s) ... aperte qualquer tecla para o proximo passo.\n",
                    teclasDoMovimento(mov).c_str(), NOME_MOVIMENTO[(size_t) mov].c_str());
        lerTecla();
    }
    temSolucao = false;
    mensagem = "Solucao aplicada - o cubo esta resolvido.";
}

// Devolve false quando o usuario pede para sair.
static bool tratarTecla(int tecla)
{
    switch (tecla) {
    case 'u': jogar(0); return true;
    case 'U': jogar(2); return true;
    case 'r': jogar(3); return true;
    case 'R': jogar(5); return true;
    case 'f': jogar(6); return true;
    case 'F': jogar(8); return true;

    case 'z': desfazer(); return true;

    case 'e': embaralharAgora(); return true;

    case 'v':
        modoIsometrico = !modoIsometrico;
        mensagem = modoIsometrico
                       ? "Vista de canto (3 faces: U, F, R)."
                       : "Planificacao completa (6 faces).";
        return true;

#ifdef COM_JANELA_3D
    case 'j':
        abrirJanela3D(cubo, historico, temSolucao, ultimaBusca);
        mensagem = "De volta ao console.";
        return true;
#endif

    case 'c':
        cubo = cuboResolvido();
        historico.clear();
        temSolucao = false;
        mensagem = "Cubo voltou ao estado resolvido.";
        return true;

    case 'C': resetarPrograma(); return true;

    case '1': resolver(0); return true;
    case '2': resolver(1); return true;
    case '3': resolver(2); return true;
    case 'm': compararTodas(); return true;

    case 'a': aplicarSolucao(); return true;

    case 'q': return false;

    default: return true;
    }
}

int main()
{
    habilitarCoresNoTerminal();

    bool rodando = true;
    while (rodando) {
        desenharTela();
        int tecla = lerTecla();
        rodando = tratarTecla(tecla);
    }

    limparTela();
    std::printf("\n  Ate mais!\n\n");
    return 0;
}
