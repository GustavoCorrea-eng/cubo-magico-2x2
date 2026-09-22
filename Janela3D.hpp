// ============================================================
// Janela3D.hpp - Visualizacao 3D de verdade, com raylib
//
// Isto e um EXTRA (o enunciado permite: "interface 3D"). O
// programa continua funcionando 100% sem isto - so compila se
// voce pedir o alvo com raylib (veja o Makefile, alvo "3d").
//
// Abre uma janela grafica com o cubo em 3D, com camera que gira
// (setas do teclado) e giros animados. Reaproveita o MESMO cubo e
// historico da sessao do console: o que voce fizer aqui continua
// valendo quando a janela fecha e o console volta ao ar.
// ============================================================
#pragma once
#include <vector>
#include "Busca.hpp"
#include "Cubo.hpp"

// Abre a janela e so devolve o controle quando o usuario a fecha
// (tecla ESC/q ou o X da janela). "ultimaBusca"/"temSolucao" sao
// usados se o usuario apertar espaco para aplicar a solucao ja
// encontrada, com animacao.
void abrirJanela3D(Cubo &cubo, std::vector<int> &historico,
                   bool &temSolucao, Resultado &ultimaBusca);
