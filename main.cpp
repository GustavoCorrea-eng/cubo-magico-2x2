// ============================================================
// main.cpp - Cubo magico 2x2x2: interface de texto colorida
//
// O jogador pode girar o cubo com uma unica tecla (sem apertar
// Enter), ou pedir para uma das tres IAs resolver e mostrar o
// caminho encontrado.
//
// Estrutura do programa: o main() repete tres passos - desenha a
// tela, espera UMA tecla e executa a acao daquela tecla - ate o
// usuario apertar 'q'. Todo o estado da sessao fica em variaveis
// estaticas (visiveis so neste arquivo).
// ============================================================
// toupper(): usado para converter a letra de um movimento em maiuscula.
#include <cctype>
// printf / fflush: saida formatada no terminal.
#include <cstdio>
// cin / cout: leitura dos numeros digitados ao embaralhar.
#include <iostream>
#include <string>
#include "Busca.hpp"
#include "Cubo.hpp"
#include "Teclado.hpp"
// A janela 3D so existe se o programa for compilado com -DCOM_JANELA_3D (make 3d).
#ifdef COM_JANELA_3D
#include "Janela3D.hpp"
#endif

// Valores iniciais da sessao (tambem usados pelo reset completo, tecla 'C').
static const unsigned int SEMENTE_PADRAO       = 2024;
static const int          TAMANHO_PADRAO       = 9;
static const char        *MENSAGEM_INICIAL     = "Aperte 'e' para embaralhar, ou jogue com u r f.";

// O cubo atual (comeca resolvido).
static Cubo cubo = cuboResolvido();
// Lista dos movimentos feitos pelo jogador (usada para contar e para desfazer).
static std::vector<int> historico;
// Semente e quantidade de movimentos da proxima vez que embaralhar.
static unsigned int semente = SEMENTE_PADRAO;
static int tamanhoEmbaralho = TAMANHO_PADRAO;
// Texto mostrado na ultima linha da tela ("o que acabou de acontecer").
static std::string mensagem = MENSAGEM_INICIAL;

// Verdadeiro quando existe uma solucao valida guardada para o cubo ATUAL.
static bool temSolucao = false;
// A ultima solucao calculada por uma das buscas.
static Resultado ultimaBusca;
static bool modoIsometrico = false;   // false = planificacao (6 faces), true = vista de canto (3 faces)

// Apaga a tela do terminal e leva o cursor para o canto superior esquerdo (codigos ANSI).
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
    // Tecla de cada face, na ordem dos codigos: U, R, F.
    static const char LETRA[3] = {'u', 'r', 'f'};
    int face = mov / 3, variacao = mov % 3;
    char horario = LETRA[face];
    char antiHorario = (char) toupper(horario);

    if (variacao == 0) return std::string(1, horario);
    if (variacao == 2) return std::string(1, antiHorario);
    return std::string(1, horario) + " " + std::string(1, horario);   // 180 graus
}

// Junta as teclas de todos os passos da solucao num texto unico,
// separando os passos por dois espacos.
static std::string nomesDosPassos(const std::vector<int> &passos)
{
    std::string s;
    for (size_t i = 0; i < passos.size(); i++) {
        if (i) s += "  ";
        s += teclasDoMovimento(passos[i]);
    }
    // Lista vazia significa que o cubo ja estava resolvido.
    return s.empty() ? "(nenhum - ja estava resolvido)" : s;
}

// Redesenha a tela inteira: o cubo, as informacoes, a ultima busca,
// o menu de teclas e a mensagem.
static void desenharTela()
{
    limparTela();
    std::printf("  =============== CUBO MAGICO 2x2x2 ===============\n");
    // Escolhe a vista do cubo: de canto (3 faces) ou planificada (6 faces).
    if (modoIsometrico) imprimirCuboIso(cubo);
    else                imprimirCubo(cubo);

    // Informacoes do estado atual.
    std::printf("   Movimentos feitos ... %zu\n", historico.size());
    std::printf("   Semente .............. %u  (%d movimentos ao embaralhar)\n",
                semente, tamanhoEmbaralho);
    std::printf("   Situacao ............. %s\n\n",
                estaResolvido(cubo) ? "RESOLVIDO" : "embaralhado");

    // Resultado da ultima busca, se ainda for valido para este cubo.
    if (temSolucao) {
        if (ultimaBusca.encontrou) {
            std::printf("   Ultima busca: %zu movimento(s), %lu estados visitados, %.3fs\n",
                        ultimaBusca.passos.size(), ultimaBusca.visitados, ultimaBusca.segundos);
            std::printf("   Solucao (teclas, na ordem): %s\n\n", nomesDosPassos(ultimaBusca.passos).c_str());
        } else {
            std::printf("   Ultima busca: sem solucao dentro do limite testado.\n\n");
        }
    }

    // Menu de teclas: "%-12s %-9s %s" alinha categoria, tecla e descricao em colunas fixas.
    std::printf("   ---------------------------------------------------------------\n");
    std::printf("    %-12s %-9s %s\n", "JOGAR",      "u r f",   "giro horario");
    std::printf("    %-12s %-9s %s\n", "",           "U R F",   "giro anti-horario (shift)");
    std::printf("    %-12s %-9s %s\n", "",           "z",       "desfazer");
    std::printf("    %-12s %-9s %s\n", "VISUALIZAR", "v",       "alternar planificacao / vista de canto");
#ifdef COM_JANELA_3D
    std::printf("    %-12s %-9s %s\n", "",           "j",       "abrir janela 3D de verdade (raylib)");
#endif
    std::printf("    %-12s %-9s %s\n", "EMBARALHAR", "e",       "nova semente / quantidade de movimentos");
    std::printf("    %-12s %-9s %s\n", "",           "c",       "cubo volta ao resolvido");
    std::printf("    %-12s %-9s %s\n", "IA",         "1 2 3",   "resolver com Largura / Prof. Iterativa / A*");
    std::printf("    %-12s %-9s %s\n", "",           "m",       "rodar as 3 de uma vez e comparar numa tabela");
    std::printf("    %-12s %-9s %s\n", "",           "a",       "aplicar a solucao encontrada");
    std::printf("    %-12s %-9s %s\n", "PROGRAMA",   "C",       "reiniciar tudo (shift+c)");
    std::printf("    %-12s %-9s %s\n", "",           "q",       "sair");
    std::printf("   ---------------------------------------------------------------\n");
    std::printf("   >> %s\n", mensagem.c_str());
}

// ------------------------------------------------------------
// Aplica um movimento feito pelo jogador.
static void jogar(int mov)
{
    cubo = mover(cubo, mov);
    // Registra no historico para poder desfazer depois.
    historico.push_back(mov);
    // O cubo mudou, entao qualquer solucao guardada deixou de valer.
    temSolucao = false;
    mensagem = "Movimento '" + teclasDoMovimento(mov) + "' aplicado.";
}

// Desfaz o ultimo movimento: aplica o movimento inverso e o tira do historico.
static void desfazer()
{
    if (historico.empty()) { mensagem = "Nada para desfazer."; return; }
    int ultimo = historico.back();
    historico.pop_back();
    cubo = mover(cubo, movimentoInverso(ultimo));
    // O cubo mudou, entao qualquer solucao guardada deixou de valer.
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
    // Resultado() cria um resultado vazio, descartando a busca antiga.
    ultimaBusca = Resultado();
    semente = SEMENTE_PADRAO;
    tamanhoEmbaralho = TAMANHO_PADRAO;
    modoIsometrico = false;
    mensagem = "Programa reiniciado.";
}

// Pede a semente e a quantidade de movimentos e embaralha o cubo.
// Aqui a leitura e com cin (numero + Enter), diferente do resto do
// programa, que le uma tecla so.
static void embaralharAgora()
{
    std::cout << "\n   Digite a semente (numero inteiro): ";
    std::cout.flush();
    unsigned int novaSemente;
    // Se o usuario digitar algo invalido, mantem a semente anterior.
    if (std::cin >> novaSemente) semente = novaSemente;

    std::cout << "   Quantos movimentos (1-40)? ";
    std::cout.flush();
    int novoTamanho;
    // So aceita valores entre 1 e 40; fora disso, mantem o anterior.
    if (std::cin >> novoTamanho && novoTamanho >= 1 && novoTamanho <= 40)
        tamanhoEmbaralho = novoTamanho;

    embaralhar(cubo, semente, tamanhoEmbaralho);
    // Cubo novo: zera o historico e descarta qualquer solucao antiga.
    historico.clear();
    temSolucao = false;
    mensagem = "Embaralhado com a semente " + std::to_string(semente) + ".";
}

// Roda uma das buscas no cubo atual: 0 = Largura, 1 = Profundidade
// Iterativa, 2 = A*. O resultado fica guardado em ultimaBusca.
static void resolver(int estrategia)
{
    // Nomes usados nas mensagens, na mesma ordem dos numeros acima.
    static const char *nomes[3] = {
        "Busca em Largura", "Profundidade Limitada Iterativa", "A*"
    };

    if (estaResolvido(cubo)) {
        mensagem = "O cubo ja esta resolvido.";
        return;
    }

    // A busca roda de forma sincrona: a tela fica parada ate ela terminar.
    std::printf("\n   Rodando %s...\n", nomes[estrategia]);
    if (estrategia == 0)
        std::printf("   (a busca em largura visita muitos estados - pode levar alguns segundos)\n");
    std::fflush(stdout);

    // Chama a busca escolhida (o 11 e o limite maximo de profundidade
    // da iterativa: nenhum estado do 2x2x2 precisa de mais de 11 movimentos).
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

    // Roda as tres buscas em sequencia, mostrando o progresso de cada uma.
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

    // Tabela comparativa (as larguras de coluna alinham os numeros).
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

    // Quantas vezes o A* visitou menos estados que as outras duas.
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

// Aplica no cubo, passo a passo, a solucao encontrada pela ultima busca.
// A cada passo redesenha a tela e espera uma tecla para continuar.
static void aplicarSolucao()
{
    // So aplica se a solucao ainda for valida para o cubo atual.
    if (!temSolucao || !ultimaBusca.encontrou) {
        mensagem = "Rode uma busca antes (teclas 1, 2 ou 3).";
        return;
    }

    for (int mov : ultimaBusca.passos) {
        cubo = mover(cubo, mov);
        historico.push_back(mov);
        desenharTela();
        // Mostra o passo tanto em teclas quanto na notacao classica de cubo.
        std::printf("\n   Aplicando '%s' (notacao classica: %s) ... aperte qualquer tecla para o proximo passo.\n",
                    teclasDoMovimento(mov).c_str(), NOME_MOVIMENTO[(size_t) mov].c_str());
        lerTecla();
    }
    // A solucao foi usada por inteiro; nao pode ser aplicada de novo.
    temSolucao = false;
    mensagem = "Solucao aplicada - o cubo esta resolvido.";
}

// Executa a acao da tecla apertada.
// Devolve false quando o usuario pede para sair.
static bool tratarTecla(int tecla)
{
    switch (tecla) {
    // Giros: minuscula = horario (codigos 0, 3, 6), maiuscula = anti-horario (2, 5, 8).
    case 'u': jogar(0); return true;
    case 'U': jogar(2); return true;
    case 'r': jogar(3); return true;
    case 'R': jogar(5); return true;
    case 'f': jogar(6); return true;
    case 'F': jogar(8); return true;

    case 'z': desfazer(); return true;

    case 'e': embaralharAgora(); return true;

    // Alterna entre a planificacao e a vista de canto.
    case 'v':
        modoIsometrico = !modoIsometrico;
        mensagem = modoIsometrico
                       ? "Vista de canto (3 faces: U, F, R)."
                       : "Planificacao completa (6 faces).";
        return true;

    // Abre a janela 3D (so existe na versao compilada com make 3d). O
    // programa fica dentro dessa chamada ate a janela ser fechada.
#ifdef COM_JANELA_3D
    case 'j':
        abrirJanela3D(cubo, historico, temSolucao, ultimaBusca);
        mensagem = "De volta ao console.";
        return true;
#endif

    // 'c' so volta o cubo ao estado resolvido.
    case 'c':
        cubo = cuboResolvido();
        historico.clear();
        temSolucao = false;
        mensagem = "Cubo voltou ao estado resolvido.";
        return true;

    // 'C' (shift+c) reinicia o programa inteiro.
    case 'C': resetarPrograma(); return true;

    // Buscas: 1 = Largura, 2 = Profundidade Iterativa, 3 = A*.
    case '1': resolver(0); return true;
    case '2': resolver(1); return true;
    case '3': resolver(2); return true;
    case 'm': compararTodas(); return true;

    case 'a': aplicarSolucao(); return true;

    // Unica tecla que encerra o programa.
    case 'q': return false;

    // Qualquer outra tecla e ignorada.
    default: return true;
    }
}

// Ponto de entrada: repete "desenhar -> ler tecla -> agir" ate sair.
int main()
{
    // Liga as cores ANSI no console do Windows.
    habilitarCoresNoTerminal();

    bool rodando = true;
    while (rodando) {
        desenharTela();
        // Espera uma tecla (sem precisar de Enter).
        int tecla = lerTecla();
        rodando = tratarTecla(tecla);
    }

    limparTela();
    std::printf("\n  Ate mais!\n\n");
    return 0;
}
