#include "Cubo.hpp"
#include <cctype>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

const std::array<std::string, N_MOVIMENTOS> NOME_MOVIMENTO = {
    "U", "U2", "U'", "R", "R2", "R'", "F", "F2", "F'"
};

static const uint8_t PERM[3][N_CANTOS] = {
     {3, 0, 1, 2, 4, 5, 6, 7},
     {4, 1, 2, 0, 6, 5, 3, 7},
     {1, 5, 2, 3, 0, 4, 6, 7}
};
static const uint8_t TWIST[3][N_CANTOS] = {
     {0, 0, 0, 0, 0, 0, 0, 0},
     {2, 0, 0, 1, 1, 0, 2, 0},
     {1, 2, 0, 0, 2, 1, 0, 0}
};

Cubo cuboResolvido()
{
    Cubo c;
    for (int i = 0; i < N_CANTOS; i++) { c.cp[i] = (uint8_t) i; c.co[i] = 0; }
    return c;
}

bool estaResolvido(const Cubo &c)
{
    for (int i = 0; i < N_CANTOS; i++)
        if (c.cp[i] != i || c.co[i] != 0) return false;
    return true;
}

static Cubo umQuartoDeVolta(const Cubo &o, int face)
{
    Cubo d;
    for (int i = 0; i < N_CANTOS; i++) {
        int origem = PERM[face][i];
        d.cp[i] = o.cp[origem];
        d.co[i] = (uint8_t) ((o.co[origem] + TWIST[face][i]) % 3);
    }
    return d;
}

Cubo mover(const Cubo &origem, int movimento)
{
    int face  = movimento / 3;
    int vezes = movimento % 3 + 1;
    Cubo atual = origem;
    for (int k = 0; k < vezes; k++)
        atual = umQuartoDeVolta(atual, face);
    return atual;
}

uint32_t indiceDoEstado(const Cubo &c)
{
    uint32_t perm = 0, orient = 0;
    for (int i = 0; i < 7; i++) {
        int menores = 0;
        for (int j = i + 1; j < 7; j++)
            if (c.cp[j] < c.cp[i]) menores++;
        perm = perm * (uint32_t) (7 - i) + (uint32_t) menores;
    }
    for (int i = 0; i < 6; i++)
        orient = orient * 3u + c.co[i];
    return perm * 729u + orient;
}

static uint32_t proximoAleatorio(uint32_t &estado)
{
    estado ^= estado << 13;
    estado ^= estado >> 17;
    estado ^= estado << 5;
    return estado;
}

std::vector<int> embaralhar(Cubo &c, unsigned int semente, int n)
{
    std::vector<int> movimentos;
    movimentos.reserve((size_t) n);
    uint32_t rng = semente ? (uint32_t) semente : 0x9E3779B9u;
    int ultimaFace = -1;

    c = cuboResolvido();
    for (int i = 0; i < n; i++) {
        int face;
        do { face = (int) (proximoAleatorio(rng) % 3u); } while (face == ultimaFace);
        int mov = face * 3 + (int) (proximoAleatorio(rng) % 3u);
        ultimaFace = face;

        c = mover(c, mov);
        movimentos.push_back(mov);
    }
    return movimentos;
}

int movimentoPorNome(const std::string &s)
{
    if (s.empty()) return -1;
    int face;
    char c = (char) toupper((unsigned char) s[0]);
    if      (c == 'U') face = 0;
    else if (c == 'R') face = 1;
    else if (c == 'F') face = 2;
    else return -1;

    int variacao;
    if (s.size() == 1)                                  variacao = 0;
    else if (s.size() == 2 && s[1] == '2')               variacao = 1;
    else if (s.size() == 2 && (s[1] == '\'' || s[1] == '3')) variacao = 2;
    else return -1;

    return face * 3 + variacao;
}

bool mesmaFace(int movA, int movB) { return movA / 3 == movB / 3; }

int movimentoInverso(int mov)
{
    int face = mov / 3, v = mov % 3;
    if (v == 1) return mov;
    return face * 3 + (v == 0 ? 2 : 0);
}

const uint8_t CANTO_FACELET[N_CANTOS][3] = {
     { 3,  4,  9},
     { 2,  8, 17},
     { 0, 16, 21},
     { 1, 20,  5},
     {13, 11,  6},
     {12, 19, 10},
     {15,  7, 22},
     {14, 23, 18}
};

static const uint8_t COR_PECA[N_CANTOS][3] = {
    {0, 1, 2}, {0, 2, 4}, {0, 4, 5}, {0, 5, 1},
    {3, 2, 1}, {3, 4, 2}, {3, 1, 5}, {3, 5, 4}
};

std::array<uint8_t, 24> facelets(const Cubo &c)
{
    std::array<uint8_t, 24> f{};
    for (int i = 0; i < N_CANTOS; i++) {
        int peca = c.cp[i], ori = c.co[i];
        for (int n = 0; n < 3; n++)
            f[CANTO_FACELET[i][(n + ori) % 3]] = COR_PECA[peca][n];
    }
    return f;
}

static const char LETRA_FACE[6] = {'U', 'R', 'F', 'D', 'L', 'B'};
static const char *COR_ANSI[6] = {
    "\x1b[47;30m",
    "\x1b[41;97m",
    "\x1b[42;30m",
    "\x1b[103;30m",
    "\x1b[48;5;208;30m",
    "\x1b[44;97m"
};

static void imprimirAdesivo(uint8_t cor)
{
    std::printf("%s %c \x1b[0m", COR_ANSI[cor], LETRA_FACE[cor]);
}

void imprimirCubo(const Cubo &c)
{
    auto f = facelets(c);
    static const int LINHA_MEIO[2][8] = {
        {16, 17,  8,  9,  4,  5, 20, 21},
        {18, 19, 10, 11,  6,  7, 22, 23}
    };

    std::printf("\n            ");
    imprimirAdesivo(f[0]); imprimirAdesivo(f[1]);
    std::printf("\n            ");
    imprimirAdesivo(f[2]); imprimirAdesivo(f[3]);
    std::printf("\n");

    for (int i = 0; i < 2; i++) {
        std::printf("  ");
        for (int k = 0; k < 8; k++) {
            imprimirAdesivo(f[LINHA_MEIO[i][k]]);
            if (k % 2 == 1) std::printf(" ");
        }
        std::printf("\n");
    }

    std::printf("            ");
    imprimirAdesivo(f[12]); imprimirAdesivo(f[13]);
    std::printf("\n            ");
    imprimirAdesivo(f[14]); imprimirAdesivo(f[15]);
    std::printf("\n\n");
    std::printf("   (L)      (F)      (R)      (B)   -  U em cima, D embaixo\n\n");
}

void imprimirCuboIso(const Cubo &c)
{
    auto f = facelets(c);

    std::printf("\n        ");
    imprimirAdesivo(f[0]); imprimirAdesivo(f[1]);
    std::printf("     (U em cima)\n");

    std::printf("      ");
    imprimirAdesivo(f[2]); imprimirAdesivo(f[3]);
    std::printf("\n");

    imprimirAdesivo(f[8]); imprimirAdesivo(f[9]);
    std::printf("  ");
    imprimirAdesivo(f[4]); imprimirAdesivo(f[5]);
    std::printf("\n");

    imprimirAdesivo(f[10]); imprimirAdesivo(f[11]);
    std::printf("  ");
    imprimirAdesivo(f[6]); imprimirAdesivo(f[7]);
    std::printf("   (F na frente, R na direita)\n\n");
    std::printf("   (as faces D, L e B ficam escondidas atras, como num cubo de verdade)\n\n");
}

void habilitarCoresNoTerminal()
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &modo))
        SetConsoleMode(h, modo | 0x0004 );
#endif
}
