// ============================================================
// Teclado.cpp - leitura de uma unica tecla, sem Enter.
// Ha duas versoes da funcao lerTecla(): uma para Windows e outra
// para Linux/macOS; o compilador escolhe a certa em #ifdef _WIN32.
// ============================================================
#include "Teclado.hpp"

// ---------- versao para Windows ----------
#ifdef _WIN32
// conio.h traz _getch(): le uma tecla direto do teclado, sem eco e sem Enter.
#include <conio.h>

int lerTecla()
{
    // Espera (bloqueia) ate uma tecla ser apertada.
    int c = _getch();
    // Setas e teclas de funcao mandam 2 bytes: um codigo especial (0 ou 224)
    // e depois o codigo real. Le e joga fora o segundo para nao "sobrar"
    // no buffer e ser confundido com uma tecla comum.
    if (c == 0 || c == 224) _getch();   // descarta o 2o byte de teclas especiais
    return c;
}

// ---------- versao para Linux / macOS ----------
#else
// termios: configuracao do terminal (modo linha, eco de teclas...).
#include <termios.h>
// read() e STDIN_FILENO: leitura direta da entrada padrao.
#include <unistd.h>

int lerTecla()
{
    // Guarda a configuracao atual do terminal para restaurar depois.
    termios antiga{}, nova{};
    tcgetattr(STDIN_FILENO, &antiga);
    nova = antiga;
    // Desliga o "modo linha" (que so entrega o texto apos o Enter) e o eco
    // (que mostraria a tecla digitada na tela).
    nova.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &nova);

    // Le exatamente 1 caractere; se a leitura falhar, devolve 0.
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) c = 0;

    // Devolve o terminal ao modo normal.
    tcsetattr(STDIN_FILENO, TCSANOW, &antiga);
    return c;
}

#endif
