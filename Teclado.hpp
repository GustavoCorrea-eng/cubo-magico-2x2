// ============================================================
// Teclado.hpp - Le UMA tecla do terminal, sem precisar de Enter
//
// No Windows usa _getch() (conio.h). Em outros sistemas coloca o
// terminal em modo bruto so durante a leitura de uma tecla e
// devolve ao normal em seguida - nao precisa ficar em modo bruto
// o programa inteiro, porque aqui a leitura pode bloquear
// (esperar o usuario apertar algo) sem problema.
// ============================================================
#pragma once

// Bloqueia ate o usuario apertar uma tecla e devolve o caractere.
int lerTecla();
