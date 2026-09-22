#include "Teclado.hpp"

#ifdef _WIN32
#include <conio.h>

int lerTecla()
{
    int c = _getch();
    if (c == 0 || c == 224) _getch();   // descarta o 2o byte de teclas especiais
    return c;
}

#else
#include <termios.h>
#include <unistd.h>

int lerTecla()
{
    termios antiga{}, nova{};
    tcgetattr(STDIN_FILENO, &antiga);
    nova = antiga;
    nova.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &nova);

    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) c = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &antiga);
    return c;
}

#endif
