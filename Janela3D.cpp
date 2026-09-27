// ============================================================
// Janela3D.cpp - janela 3D do cubo, feita com a biblioteca raylib.
// So e compilado no alvo "make 3d" (macro COM_JANELA_3D).
//
// Ideia geral: a cada quadro (60 por segundo) a funcao le o teclado,
// atualiza a animacao do giro em andamento, e desenha os 8 cantos do
// cubo (cada um e um cubinho preto com ate 3 adesivos coloridos).
// ============================================================
#include "Janela3D.hpp"
// cosf / sinf: usados para posicionar a camera em volta do cubo.
#include <cmath>
// raylib.h: janela, teclado e desenho; rlgl.h: pilha de matrizes (rotacao das pecas).
#include <raylib.h>
#include <rlgl.h>

// Define PI caso a biblioteca ainda nao tenha definido.
#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ------------------------------------------------------------
// Geometria: onde fica cada um dos 8 cantos no espaco, e para
// onde apontam os seus 3 adesivos visiveis. As mesmas duas
// tabelas descrevem tanto a posicao quanto quais adesivos
// pertencem a cada canto - "DIR" segue a MESMA ordem de
// CANTO_FACELET (em Cubo.cpp), entao da pra buscar a cor direto.
//
//   x+ = direita (R)   y+ = cima (U)   z+ = frente (F)
// ------------------------------------------------------------
// POS: em qual "quadrante" do espaco (sinal de x, y, z) esta cada canto.
static const float POS[8][3] = {
    { 1,  1,  1},   // 0 URF
    {-1,  1,  1},   // 1 UFL
    {-1,  1, -1},   // 2 ULB
    { 1,  1, -1},   // 3 UBR
    { 1, -1,  1},   // 4 DFR
    {-1, -1,  1},   // 5 DLF
    { 1, -1, -1},   // 6 DRB
    {-1, -1, -1}    // 7 DBL
};

// DIR: para cada canto, os 3 vetores "para fora" das faces que aparecem
// (cada linha {x, y, z} tem um unico valor diferente de zero, indicando o eixo).
static const float DIR[8][3][3] = {
    {{0, 1, 0}, { 1, 0,  0}, { 0, 0,  1}},
    {{0, 1, 0}, { 0, 0,  1}, {-1, 0,  0}},
    {{0, 1, 0}, {-1, 0,  0}, { 0, 0, -1}},
    {{0, 1, 0}, { 0, 0, -1}, { 1, 0,  0}},
    {{0,-1, 0}, { 0, 0,  1}, { 1, 0,  0}},
    {{0,-1, 0}, {-1, 0,  0}, { 0, 0,  1}},
    {{0,-1, 0}, { 1, 0,  0}, { 0, 0, -1}},
    {{0,-1, 0}, { 0, 0, -1}, {-1, 0,  0}}
};

// Cor (RGBA) de cada face, na ordem U, R, F, D, L, B.
static const Color CORES[6] = {
    Color{245, 245, 245, 255},   // U branco
    Color{205,  50,  50, 255},   // R vermelho
    Color{ 45, 170,  85, 255},   // F verde
    Color{245, 215,  65, 255},   // D amarelo
    Color{235, 130,  40, 255},   // L laranja
    Color{ 50,  95, 205, 255}    // B azul
};
// Cor do corpo (plastico preto) de cada pecinha e cor do fundo da janela.
static const Color PLASTICO = Color{25, 25, 30, 255};
static const Color FUNDO    = Color{24, 26, 34, 255};

static const float ARESTA = 0.94f;    // tamanho de cada pecinha (deixa uma folga = "grade" preta)
static const float ADESIVO_MARGEM = 0.14f; // quanto o adesivo e menor que a face da pecinha

// Pecas que giram em cada face: U mexe nas de cima, R nas da
// direita, F nas da frente (mesma regra usada no resto do projeto).
// Uma peca esta "em cima" quando a sua coordenada y e positiva, e assim por diante.
static bool pecaGira(int canto, int face)
{
    if (face == 0) return POS[canto][1] > 0;   // U
    if (face == 1) return POS[canto][0] > 0;   // R
    return POS[canto][2] > 0;                  // F
}

// Desenha um dos 8 cantos: o corpo preto e os 3 adesivos coloridos.
// Se "gira" for verdadeiro, o canto ja deve estar desenhado dentro
// de um rlPushMatrix/rlRotatef (feito por quem chama).
// "adesivos" traz a cor de cada uma das 24 figurinhas (vinda de facelets()).
static void desenharCanto(int i, const std::array<uint8_t, 24> &adesivos)
{
    // O centro do cubinho e a posicao do canto reduzida a metade.
    Vector3 centro = {POS[i][0] * 0.5f, POS[i][1] * 0.5f, POS[i][2] * 0.5f};
    // Corpo preto do cubinho.
    DrawCube(centro, ARESTA, ARESTA, ARESTA, PLASTICO);

    // Cada adesivo e uma caixa bem fina colada numa das 3 faces visiveis.
    for (int d = 0; d < 3; d++) {
        // n = direcao para onde essa face aponta.
        Vector3 n = {DIR[i][d][0], DIR[i][d][1], DIR[i][d][2]};
        // Centro do adesivo: um pouquinho para fora da superficie do cubinho.
        Vector3 c = {centro.x + n.x * (ARESTA / 2 + 0.012f),
                     centro.y + n.y * (ARESTA / 2 + 0.012f),
                     centro.z + n.z * (ARESTA / 2 + 0.012f)};
        // Espessura minima (0.02) no eixo da face; nos outros dois eixos o
        // adesivo cobre quase toda a face, menos uma margem.
        float largura = (n.x != 0) ? 0.02f : ARESTA - ADESIVO_MARGEM;
        float altura  = (n.y != 0) ? 0.02f : ARESTA - ADESIVO_MARGEM;
        float prof    = (n.z != 0) ? 0.02f : ARESTA - ADESIVO_MARGEM;

        // A cor vem direto da tabela de figurinhas do estado atual do cubo.
        Color cor = CORES[adesivos[CANTO_FACELET[i][d]]];
        DrawCube(c, largura, altura, prof, cor);
    }
}

// Angulo (em graus) que a camada animada ja girou, dado o progresso
// de 0 a 1 e a variacao do movimento (0=90 horario, 1=180, 2=90 anti).
static float anguloGraus(int variacao, float progresso)
{
    // Angulo final do movimento (o sinal define o sentido do giro).
    float alvo = (variacao == 0) ? -90.0f : (variacao == 1) ? -180.0f : 90.0f;
    // No inicio (progresso 0) vale 0; no fim (progresso 1) vale o angulo inteiro.
    return alvo * progresso;
}

// Abre a janela e fica nela ate o usuario fechar. Ver Janela3D.hpp.
void abrirJanela3D(Cubo &cubo, std::vector<int> &historico,
                   bool &temSolucao, Resultado &ultimaBusca)
{
    // Tamanho da janela em pixels.
    const int LARGURA = 720, ALTURA = 720;

    // Cria a janela e limita a animacao a 60 quadros por segundo.
    InitWindow(LARGURA, ALTURA, "Cubo Magico 2x2x2 - vista 3D (raylib)");
    SetTargetFPS(60);

    // Camera: yaw = giro horizontal, pitch = giro vertical, distancia = quao
    // longe do cubo. As setas do teclado alteram yaw e pitch.
    float yaw = 0.7f, pitch = 0.45f, distancia = 6.2f;
    Camera3D camera{};
    camera.target     = {0, 0, 0};                // olha para o centro do cubo
    camera.up         = {0, 1, 0};                // "para cima" e o eixo y
    camera.fovy       = 32.0f;                    // abertura da lente, em graus
    camera.projection = CAMERA_PERSPECTIVE;       // perspectiva (longe fica menor)

    // fila de movimentos a animar (usada tambem para tocar a solucao)
    std::vector<int> fila;
    // Posicao, na fila, do movimento que esta girando agora.
    size_t filaPos = 0;
    // Codigo do movimento em animacao (-1 = nenhum).
    int movAnimando = -1;
    // Ha quantos segundos o giro atual comecou.
    float tempoGiro = 0.0f;
    const float DURACAO = 0.16f;   // segundos por movimento

    // um "desfazer" anima um movimento tambem, mas NAO deve voltar a
    // entrar no historico (ja foi removido de la antes de comecar a girar)
    bool animacaoEhDesfazer = false;
    bool tocandoSolucao = false;   // true enquanto a fila e a solucao inteira (tecla espaco)

    // So e chamada para um movimento NOVO iniciado pelo jogador (u/r/f ou o
    // 'z' de desfazer) - a tecla espaco (tocar a solucao) nunca passa por
    // aqui, ela mexe em fila/movAnimando diretamente. Por isso da pra
    // invalidar a solucao guardada bem aqui, num lugar so: qualquer
    // movimento que NAO seja "aplicar a solucao em si" torna essa solucao
    // obsoleta (ela foi calculada para o cubo de ANTES desse movimento) -
    // o mesmo cuidado que jogar()/desfazer() ja tem no console (main.cpp).
    auto comecarMovimento = [&](int mov, bool ehDesfazer) {
        if (movAnimando >= 0) return;      // ja esta girando algo
        // Monta uma fila de um unico movimento e comeca a animar.
        fila.clear();
        fila.push_back(mov);
        filaPos = 0;
        movAnimando = mov;
        tempoGiro = 0.0f;
        animacaoEhDesfazer = ehDesfazer;
        temSolucao = false;
    };

    // Aplica de vez o movimento que esta animando (cubo + historico) e
    // passa para o proximo da fila, se houver. E o mesmo "commit" que
    // roda a cada frame quando a animacao termina (mais abaixo) - extraido
    // em funcao para poder ser chamado de novo, sem esperar a animacao,
    // se a janela fechar no meio de um giro (ver depois do laco principal).
    auto concluirMovimentoAtual = [&]() {
        // Aplica o movimento no cubo de verdade.
        cubo = mover(cubo, movAnimando);
        // Registra no historico (menos quando era um desfazer).
        if (!animacaoEhDesfazer) historico.push_back(movAnimando);
        movAnimando = -1;
        tempoGiro = 0.0f;
        filaPos++;
        if (filaPos < fila.size()) {
            // Ainda ha movimentos na fila: comeca o proximo.
            movAnimando = fila[filaPos];
            tempoGiro = 0.0f;
        } else if (tocandoSolucao) {
            // Acabou a fila e ela era a solucao: nao ha mais solucao pendente.
            tocandoSolucao = false;
            temSolucao = false;   // solucao inteira acabou de ser aplicada
        }
    };

    // Laco principal: cada volta e um quadro da animacao.
    // WindowShouldClose() vira verdadeiro com ESC ou ao clicar no X.
    while (!WindowShouldClose()) {
        // Tempo (em segundos) desde o quadro anterior.
        float dt = GetFrameTime();

        // --- camera: setas orbitam ao redor do cubo ---
        if (IsKeyDown(KEY_LEFT))  yaw -= 1.6f * dt;
        if (IsKeyDown(KEY_RIGHT)) yaw += 1.6f * dt;
        if (IsKeyDown(KEY_UP))    pitch += 1.3f * dt;
        if (IsKeyDown(KEY_DOWN))  pitch -= 1.3f * dt;
        // Limita a inclinacao vertical para a camera nao "virar de cabeca para baixo".
        if (pitch >  1.45f) pitch =  1.45f;
        if (pitch < -1.45f) pitch = -1.45f;
        // Converte os angulos (yaw, pitch) e a distancia em uma posicao 3D da camera.
        camera.position.x = distancia * cosf(pitch) * sinf(yaw);
        camera.position.y = distancia * sinf(pitch);
        camera.position.z = distancia * cosf(pitch) * cosf(yaw);

        // --- teclado: giros, desfazer, aplicar solucao, sair ---
        // Shift apertado = sentido anti-horario.
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        // IsKeyPressed dispara so uma vez por apertada (nao repete se segurar).
        if (IsKeyPressed(KEY_U)) comecarMovimento(shift ? 2 : 0, false);
        if (IsKeyPressed(KEY_R)) comecarMovimento(shift ? 5 : 3, false);
        if (IsKeyPressed(KEY_F)) comecarMovimento(shift ? 8 : 6, false);

        // Desfazer: tira o ultimo movimento do historico e anima o inverso.
        if (IsKeyPressed(KEY_Z) && movAnimando < 0 && !historico.empty()) {
            int ultimo = historico.back();
            historico.pop_back();
            comecarMovimento(movimentoInverso(ultimo), true);
            tocandoSolucao = false;
        }

        // Resetar: volta o cubo ao estado resolvido na hora, sem animacao.
        if (IsKeyPressed(KEY_C) && movAnimando < 0) {
            cubo = cuboResolvido();
            historico.clear();
            temSolucao = false;
        }

        // Espaco: comeca a tocar a solucao (so se ela ainda for valida para o cubo atual).
        if (IsKeyPressed(KEY_SPACE) && movAnimando < 0 &&
            temSolucao && ultimaBusca.encontrou && !ultimaBusca.passos.empty()) {
            fila = ultimaBusca.passos;
            filaPos = 0;
            movAnimando = fila[filaPos];
            tempoGiro = 0.0f;
            animacaoEhDesfazer = false;
            tocandoSolucao = true;
        }

        // --- avanca a animacao do giro atual ---
        if (movAnimando >= 0) {
            tempoGiro += dt;
            // Quando o tempo do giro acaba, o movimento e aplicado de vez.
            if (tempoGiro >= DURACAO) concluirMovimentoAtual();
        }

        // --- desenha ---
        // Cores das 24 figurinhas do estado atual do cubo.
        auto adesivos = facelets(cubo);
        // Qual face esta girando (0=U, 1=R, 2=F) ou -1 se nenhuma.
        int faceAnimada = (movAnimando >= 0) ? movAnimando / 3 : -1;
        // Quanto do giro ja foi feito, de 0 a 1.
        float progresso = (movAnimando >= 0) ? (tempoGiro / DURACAO) : 0.0f;
        if (progresso > 1.0f) progresso = 1.0f;
        // Angulo atual do giro, em graus.
        float ang = (movAnimando >= 0) ? anguloGraus(movAnimando % 3, progresso) : 0.0f;
        // Eixo de rotacao da face que esta girando (vetor unitario).
        float eixo[3] = {0, 0, 0};
        if (faceAnimada == 0) eixo[1] = 1;        // U gira em torno de y
        else if (faceAnimada == 1) eixo[0] = 1;   // R gira em torno de x
        else if (faceAnimada == 2) eixo[2] = 1;   // F gira em torno de z

        BeginDrawing();
        ClearBackground(FUNDO);
        // Tudo entre BeginMode3D e EndMode3D e desenhado em 3D pela camera.
        BeginMode3D(camera);
        for (int i = 0; i < 8; i++) {
            // So as pecas da face que esta girando sao rotacionadas.
            bool gira = (faceAnimada >= 0) && pecaGira(i, faceAnimada);
            if (gira) {
                // Guarda a matriz atual e aplica a rotacao so a esta peca.
                rlPushMatrix();
                rlRotatef(ang, eixo[0], eixo[1], eixo[2]);
            }
            desenharCanto(i, adesivos);
            // Restaura a matriz para as proximas pecas nao serem afetadas.
            if (gira) rlPopMatrix();
        }
        EndMode3D();

        // Textos por cima da cena 3D (titulo, contador e ajuda de teclas).
        DrawText("CUBO MAGICO 2x2x2 - vista 3D", 14, 12, 20, RAYWHITE);
        DrawText(TextFormat("Movimentos: %d   %s", (int) historico.size(),
                            estaResolvido(cubo) ? "RESOLVIDO" : "embaralhado"),
                 14, 38, 16, LIGHTGRAY);
        DrawText("u r f = horario   U R F (shift) = anti-horario   setas = camera",
                 14, ALTURA - 66, 15, LIGHTGRAY);
        DrawText("z = desfazer   c = resetar   espaco = aplicar solucao   ESC = voltar ao console",
                 14, ALTURA - 44, 15, LIGHTGRAY);
        // Aviso verde enquanto houver uma solucao valida esperando.
        if (temSolucao && ultimaBusca.encontrou)
            DrawText(TextFormat("Solucao pronta: %d movimento(s) - aperte espaco",
                                (int) ultimaBusca.passos.size()),
                     14, ALTURA - 90, 15, GREEN);
        DrawFPS(LARGURA - 90, 12);
        EndDrawing();
    }

    // A janela pode fechar (ESC, ou o X) no meio de uma animacao - por
    // exemplo, logo depois de 'z' (desfazer) ou de espaco (aplicar a
    // solucao), antes dos ~0.16s do giro terminarem. Sem isto, o cubo
    // devolvido ao console ficaria "pela metade": o historico ja teria
    // sido atualizado (em 'z') ou a solucao ja teria sido marcada como
    // aplicada, mas o cubo em si nao teria o(s) ultimo(s) giro(s). Termina
    // de aplicar tudo o que ainda estiver pendente, na hora, sem esperar
    // a animacao, para o console sempre receber um estado consistente.
    while (movAnimando >= 0) concluirMovimentoAtual();

    // Fecha a janela e libera os recursos graficos.
    CloseWindow();
}
