// ============================================================
// Cubo.cpp - implementacao do estado, da funcao sucessora, do
// embaralhamento e da impressao colorida do cubo 2x2x2.
// (As declaracoes e a explicacao geral estao em Cubo.hpp.)
// ============================================================
#include "Cubo.hpp"
// toupper(): usada para aceitar "r" e "R" ao ler o nome de um movimento.
#include <cctype>
// printf(): usado na impressao do cubo no terminal.
#include <cstdio>

// A API do Windows so e necessaria em habilitarCoresNoTerminal().
#ifdef _WIN32
#include <windows.h>
#endif

// Nomes dos 9 movimentos, na ordem dos codigos 0..8
// (face*3 + variacao: horario, 180 graus, anti-horario).
const std::array<std::string, N_MOVIMENTOS> NOME_MOVIMENTO = {
    "U", "U2", "U'", "R", "R2", "R'", "F", "F2", "F'"
};

// ------------------------------------------------------------
// Tabelas de UM QUARTO de volta no sentido horario (a base de
// onde saem os outros dois: 180 graus e 270 graus/anti-horario).
//
//   PERM[face][i]  = de qual posicao vem a peca que passa a
//                    ocupar a posicao i
//   TWIST[face][i] = quantos giros de 120 graus essa peca ganha
//                    ao chegar na posicao i
// ------------------------------------------------------------
// PERM: uma linha por face (U, R, F); cada coluna e uma posicao de canto.
// Ex.: em U, a peca que vai parar na posicao 0 vinha da posicao 3.
// A posicao 7 (DBL) nunca muda, por isso o ultimo valor e sempre 7.
static const uint8_t PERM[3][N_CANTOS] = {
    /* U */ {3, 0, 1, 2, 4, 5, 6, 7},
    /* R */ {4, 1, 2, 0, 6, 5, 3, 7},
    /* F */ {1, 5, 2, 3, 0, 4, 6, 7}
};
// TWIST: quanto a orientacao muda (0, 1 ou 2) quando a peca chega na
// nova posicao. Girar a face U nao torce nenhuma peca, por isso so zeros.
static const uint8_t TWIST[3][N_CANTOS] = {
    /* U */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* R */ {2, 0, 0, 1, 1, 0, 2, 0},
    /* F */ {1, 2, 0, 0, 2, 1, 0, 0}
};

// Monta o estado resolvido: a peca i esta na posicao i, sem torcao.
Cubo cuboResolvido()
{
    Cubo c;
    for (int i = 0; i < N_CANTOS; i++) { c.cp[i] = (uint8_t) i; c.co[i] = 0; }
    return c;
}

// O cubo esta resolvido quando TODAS as pecas estao na posicao certa
// e com orientacao 0. Basta uma peca fora do lugar para devolver falso.
bool estaResolvido(const Cubo &c)
{
    for (int i = 0; i < N_CANTOS; i++)
        if (c.cp[i] != i || c.co[i] != 0) return false;
    return true;
}

// Aplica UM quarto de volta horario na face indicada (0=U, 1=R, 2=F) e
// devolve o novo estado. Os outros movimentos (180 graus e anti-horario)
// sao feitos repetindo este passo 2 ou 3 vezes (ver mover()).
static Cubo umQuartoDeVolta(const Cubo &o, int face)
{
    Cubo d;
    for (int i = 0; i < N_CANTOS; i++) {
        // De qual posicao do cubo antigo vem a peca que ocupara a posicao i.
        int origem = PERM[face][i];
        // A peca simplesmente muda de lugar...
        d.cp[i] = o.cp[origem];
        // ...e sua orientacao soma a torcao da tabela (modulo 3, pois 3 giros de 120 = volta completa).
        d.co[i] = (uint8_t) ((o.co[origem] + TWIST[face][i]) % 3);
    }
    return d;
}

// FUNCAO SUCESSORA
// Recebe um estado e um movimento (0..8) e devolve o estado seguinte.
Cubo mover(const Cubo &origem, int movimento)
{
    // Separa o codigo do movimento: face*3 + variacao.
    int face  = movimento / 3;
    int vezes = movimento % 3 + 1;      // 1, 2 ou 3 quartos de volta
    Cubo atual = origem;
    // Repete o quarto de volta o numero de vezes necessario.
    for (int k = 0; k < vezes; k++)
        atual = umQuartoDeVolta(atual, face);
    return atual;
}

// ------------------------------------------------------------
// INDICE DO ESTADO: codigo de Lehmer da permutacao (0..5039) das
// 7 pecas moveis, combinado com a orientacao em base 3 (0..728).
// A peca 7 nunca se move e sua orientacao e determinada pelas
// outras (a soma das torcoes e sempre multipla de 3), entao nao
// entram na conta.
// ------------------------------------------------------------
uint32_t indiceDoEstado(const Cubo &c)
{
    uint32_t perm = 0, orient = 0;
    // Parte 1: numero da permutacao (codigo de Lehmer).
    for (int i = 0; i < 7; i++) {
        // Conta quantas pecas depois de i tem numero menor que a peca i.
        int menores = 0;
        for (int j = i + 1; j < 7; j++)
            if (c.cp[j] < c.cp[i]) menores++;
        // Acumula em "base fatorial": cada posicao vale um digito
        // que vai de 0 ate (7 - i - 1).
        perm = perm * (uint32_t) (7 - i) + (uint32_t) menores;
    }
    // Parte 2: le as orientacoes das 6 primeiras pecas como um
    // numero de 6 digitos em base 3 (a 7a e deduzida das outras).
    for (int i = 0; i < 6; i++)
        orient = orient * 3u + c.co[i];
    // Junta as duas partes num unico numero (729 = 3^6 valores de orientacao).
    return perm * 729u + orient;
}

// ------------------------------------------------------------
// Gerador pseudoaleatorio proprio (xorshift32). Nao usamos rand()
// porque o resultado varia de compilador para compilador; assim a
// mesma semente da sempre o mesmo cubo, em qualquer maquina.
// ------------------------------------------------------------
// Sorteia o proximo numero: mistura os bits do estado com deslocamentos
// e XOR (algoritmo xorshift32) e devolve o novo valor.
static uint32_t proximoAleatorio(uint32_t &estado)
{
    estado ^= estado << 13;
    estado ^= estado >> 17;
    estado ^= estado << 5;
    return estado;
}

// Embaralha o cubo c com "n" movimentos sorteados a partir da semente.
// Devolve a lista de movimentos aplicados.
std::vector<int> embaralhar(Cubo &c, unsigned int semente, int n)
{
    std::vector<int> movimentos;
    movimentos.reserve((size_t) n);
    // A semente 0 travaria o gerador (so geraria zeros), entao usa-se
    // uma constante no lugar dela.
    uint32_t rng = semente ? (uint32_t) semente : 0x9E3779B9u;
    // Guarda a face do ultimo movimento para nao repetir a mesma face
    // duas vezes seguidas (isso desperdicaria movimentos).
    int ultimaFace = -1;

    // Sempre comeca do cubo resolvido, para o resultado depender so da semente.
    c = cuboResolvido();
    for (int i = 0; i < n; i++) {
        int face;
        // Sorteia uma face (0, 1 ou 2) diferente da anterior.
        do { face = (int) (proximoAleatorio(rng) % 3u); } while (face == ultimaFace);
        // Sorteia a variacao (horario, 180 ou anti-horario) e monta o codigo.
        int mov = face * 3 + (int) (proximoAleatorio(rng) % 3u);
        ultimaFace = face;

        // Aplica o movimento e registra na lista.
        c = mover(c, mov);
        movimentos.push_back(mov);
    }
    return movimentos;
}

// Traduz um texto como "R", "U2" ou "F'" para o codigo do movimento.
// Devolve -1 se o texto for invalido.
int movimentoPorNome(const std::string &s)
{
    if (s.empty()) return -1;
    // A primeira letra escolhe a face (maiuscula ou minuscula).
    int face;
    char c = (char) toupper((unsigned char) s[0]);
    if      (c == 'U') face = 0;
    else if (c == 'R') face = 1;
    else if (c == 'F') face = 2;
    else return -1;

    // O sufixo escolhe a variacao: nada = horario, "2" = 180, "'" ou "3" = anti-horario.
    int variacao;
    if (s.size() == 1)                                  variacao = 0;
    else if (s.size() == 2 && s[1] == '2')               variacao = 1;
    else if (s.size() == 2 && (s[1] == '\'' || s[1] == '3')) variacao = 2;
    else return -1;

    return face * 3 + variacao;
}

// Dois movimentos giram a mesma face quando tem o mesmo "face*3" (divisao inteira por 3).
bool mesmaFace(int movA, int movB) { return movA / 3 == movB / 3; }

// Devolve o movimento que desfaz o movimento dado.
int movimentoInverso(int mov)
{
    int face = mov / 3, v = mov % 3;
    if (v == 1) return mov;             // o inverso de X2 e o proprio X2
    // Horario (0) vira anti-horario (2) e vice-versa.
    return face * 3 + (v == 0 ? 2 : 0);
}

// ------------------------------------------------------------
// VISUALIZACAO
//
// CANTO_FACELET[i][k] = indice (0..23) da figurinha que fica no
// k-esimo adesivo visivel da posicao de canto i. A ordem das 3
// direcoes de cada canto casa com COR_PECA (embaixo).
//
// Planificacao usada na impressao (L, F, R, B na faixa do meio):
//
//              0  1
//              2  3
//   16 17   8  9   4  5   20 21
//   18 19  10 11   6  7   22 23
//             12 13
//             14 15
// ------------------------------------------------------------
// Cada linha e uma posicao de canto (URF, UFL...) e lista os indices das
// 3 figurinhas que ela mostra na planificacao acima.
const uint8_t CANTO_FACELET[N_CANTOS][3] = {
    /* URF */ { 3,  4,  9},
    /* UFL */ { 2,  8, 17},
    /* ULB */ { 0, 16, 21},
    /* UBR */ { 1, 20,  5},
    /* DFR */ {13, 11,  6},
    /* DLF */ {12, 19, 10},
    /* DRB */ {15,  7, 22},
    /* DBL */ {14, 23, 18}
};

// Cor de cada um dos 3 adesivos de uma peca de canto SOLUCIONADA,
// na mesma ordem de CANTO_FACELET. Cores: 0=U 1=R 2=F 3=D 4=L 5=B.
// Indexada pelo numero da PECA (nao da posicao).
static const uint8_t COR_PECA[N_CANTOS][3] = {
    {0, 1, 2}, {0, 2, 4}, {0, 4, 5}, {0, 5, 1},
    {3, 2, 1}, {3, 4, 2}, {3, 1, 5}, {3, 5, 4}
};

// Converte o estado (permutacao + orientacao) nas 24 cores das figurinhas.
std::array<uint8_t, 24> facelets(const Cubo &c)
{
    std::array<uint8_t, 24> f{};
    for (int i = 0; i < N_CANTOS; i++) {
        // Qual peca esta na posicao i e com que orientacao.
        int peca = c.cp[i], ori = c.co[i];
        // Espalha os 3 adesivos da peca pelas 3 figurinhas da posicao;
        // a orientacao "gira" quais adesivos caem em quais figurinhas.
        for (int n = 0; n < 3; n++)
            f[CANTO_FACELET[i][(n + ori) % 3]] = COR_PECA[peca][n];
    }
    return f;
}

// Letra escrita dentro de cada quadradinho, indexada pela cor (0..5).
static const char LETRA_FACE[6] = {'U', 'R', 'F', 'D', 'L', 'B'};
// Codigos ANSI de cor de fundo (e de texto) de cada cor do cubo.
static const char *COR_ANSI[6] = {
    "\x1b[47;30m",        // U - branco
    "\x1b[41;97m",        // R - vermelho
    "\x1b[42;30m",        // F - verde
    "\x1b[103;30m",       // D - amarelo
    "\x1b[48;5;208;30m",  // L - laranja
    "\x1b[44;97m"         // B - azul
};

// Imprime um quadradinho colorido com a letra da cor no meio.
// "\x1b[0m" no fim desliga a cor para nao "vazar" para o texto seguinte.
static void imprimirAdesivo(uint8_t cor)
{
    std::printf("%s %c \x1b[0m", COR_ANSI[cor], LETRA_FACE[cor]);
}

// Desenha a planificacao completa: U em cima, L F R B no meio, D embaixo.
void imprimirCubo(const Cubo &c)
{
    auto f = facelets(c);
    // Indices das figurinhas das duas linhas do meio (L, F, R, B lado a lado).
    static const int LINHA_MEIO[2][8] = {
        {16, 17,  8,  9,  4,  5, 20, 21},
        {18, 19, 10, 11,  6,  7, 22, 23}
    };

    // Face U (2 linhas), deslocada para ficar em cima da face F.
    std::printf("\n            ");
    imprimirAdesivo(f[0]); imprimirAdesivo(f[1]);
    std::printf("\n            ");
    imprimirAdesivo(f[2]); imprimirAdesivo(f[3]);
    std::printf("\n");

    // Faixa do meio: cada linha tem 4 faces de 2 quadradinhos.
    for (int i = 0; i < 2; i++) {
        std::printf("  ");
        for (int k = 0; k < 8; k++) {
            imprimirAdesivo(f[LINHA_MEIO[i][k]]);
            // Depois de cada 2 quadradinhos (uma face) deixa um espaco.
            if (k % 2 == 1) std::printf(" ");
        }
        std::printf("\n");
    }

    // Face D (2 linhas), embaixo da face F.
    std::printf("            ");
    imprimirAdesivo(f[12]); imprimirAdesivo(f[13]);
    std::printf("\n            ");
    imprimirAdesivo(f[14]); imprimirAdesivo(f[15]);
    std::printf("\n\n");
    std::printf("   (L)      (F)      (R)      (B)   -  U em cima, D embaixo\n\n");
}

// ------------------------------------------------------------
// VISUALIZACAO EM "CANTO" (pseudo-3D)
//
// Mostra so as 3 faces que dariam pra ver olhando para o
// cubo de frente e de cima: U (topo), F (frente) e R (direita).
// As faces D, L e B ficam escondidas atras, como aconteceria
// numa vista 3D de verdade.
//
// Nao ha nenhuma camera nem matriz de rotacao aqui - o efeito de
// profundidade e so a face U sendo desenhada progressivamente
// mais a direita a cada linha, "recuando" para tras. E uma
// aproximacao barata de 3D, nao uma projecao real.
// ------------------------------------------------------------
void imprimirCuboIso(const Cubo &c)
{
    auto f = facelets(c);

    // Fileira de tras da face U (a mais afastada, mais deslocada para a direita).
    std::printf("\n        ");
    imprimirAdesivo(f[0]); imprimirAdesivo(f[1]);
    std::printf("     (U em cima)\n");

    // Fileira da frente da face U (um pouco menos deslocada).
    std::printf("      ");
    imprimirAdesivo(f[2]); imprimirAdesivo(f[3]);
    std::printf("\n");

    // Primeira linha das faces F (esquerda) e R (direita), lado a lado.
    imprimirAdesivo(f[8]); imprimirAdesivo(f[9]);
    std::printf("  ");
    imprimirAdesivo(f[4]); imprimirAdesivo(f[5]);
    std::printf("\n");

    // Segunda linha das faces F e R.
    imprimirAdesivo(f[10]); imprimirAdesivo(f[11]);
    std::printf("  ");
    imprimirAdesivo(f[6]); imprimirAdesivo(f[7]);
    std::printf("   (F na frente, R na direita)\n\n");
    std::printf("   (as faces D, L e B ficam escondidas atras, como num cubo de verdade)\n\n");
}

// Liga o modo que faz o console do Windows entender os codigos de cor ANSI.
// Em outros sistemas (Linux/macOS) o terminal ja entende, entao nao faz nada.
void habilitarCoresNoTerminal()
{
#ifdef _WIN32
    // Pega o "identificador" da saida padrao (a tela do console).
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo;
    // Le o modo atual do console e liga o bit de processamento de terminal virtual.
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &modo))
        SetConsoleMode(h, modo | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
#endif
}
