# Guia de Estudo — Cubo Mágico 2x2x2 (passo a passo, sem jargão)

Esse documento existe pra você estudar pra arguição sem se perder. Ele segue
o programa **do jeito que ele realmente executa**: começa no `main()`, e vai
entrando em cada arquivo só na hora que o código realmente entra lá — do
mesmo jeito que fomos fazendo na conversa.

Não é pra decorar. É pra ler com calma, uma seção de cada vez, testando
cada exemplo numérico na cabeça (ou no papel) antes de ir pra próxima.

**Como ler os nomes de variável nesse documento:** toda vez que aparecer uma
letra ou nome estranho tipo `o`, `d`, `i`, `face`, isso é sempre uma
**variável** — uma "gaveta" com um valor dentro, que muda ao longo do
código. Nunca é um nome fixo de alguma coisa do mundo real.

---

# PARTE 1 — `main.cpp`: o maestro do programa

O `main.cpp` não faz o trabalho pesado sozinho — ele é quem **organiza**:
decide quando chamar o `Cubo.cpp` (pra mexer no cubo) e quando chamar o
`Busca.cpp` (pra pedir uma solução da IA). Pensa nele como o garçom do
restaurante: não cozinha nada, mas sabe exatamente pra qual cozinha levar
cada pedido.

## 1.1 — As bibliotecas (linhas 1-10)

```cpp
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
```

Cada `#include` é uma "promessa" de que tal coisa existe em outro lugar.
As primeiras 4 linhas são bibliotecas prontas do C++ (nada que a gente
escreveu). As de baixo (`"Busca.hpp"`, `"Cubo.hpp"`, `"Teclado.hpp"`) são os
**nossos** arquivos — é por causa dessas linhas que o `main.cpp` pode usar
funções como `mover()` (que mora no `Cubo.cpp`) ou `buscaEmLargura()` (que
mora no `Busca.cpp`), mesmo sem o código delas estar neste arquivo.

`#ifdef COM_JANELA_3D ... #endif` é diferente de um `if` comum: é uma
instrução pro **compilador** (decidida na hora de gerar o `.exe`, não
enquanto o programa roda). Se você compilar com `make 3d`, essa linha entra
no programa; se compilar só com `make`, ela é like se nem existisse. É assim
que o projeto consegue funcionar dos dois jeitos, com e sem a janela 3D.

## 1.2 — As constantes de fábrica (linhas 12-14)

```cpp
static const unsigned int SEMENTE_PADRAO       = 2024;
static const int          TAMANHO_PADRAO       = 9;
static const char        *MENSAGEM_INICIAL     = "Aperte 'e' para embaralhar, ou jogue com u r f.";
```

Três valores fixos, que nunca mudam (`const`). São os valores "de fábrica"
que o programa usa antes de você mexer em qualquer coisa, e também os
valores que ele **volta a usar** quando você aperta `C` (reiniciar tudo).

## 1.3 — O estado vivo da sessão (linhas 16-24)

```cpp
static Cubo cubo = cuboResolvido();
static std::vector<int> historico;
static unsigned int semente = SEMENTE_PADRAO;
static int tamanhoEmbaralho = TAMANHO_PADRAO;
static std::string mensagem = MENSAGEM_INICIAL;

static bool temSolucao = false;
static Resultado ultimaBusca;
static bool modoIsometrico = false;
```

Essas 8 variáveis são declaradas **fora de qualquer função**, lá no topo do
arquivo. Isso é diferente de uma variável declarada dentro de uma função
(que só existe enquanto aquela função roda, e some depois): essas aqui
**vivem o tempo inteiro** que o programa roda, e **qualquer função** deste
arquivo pode ler e mudar elas diretamente, sem precisar receber como
parâmetro.

Pensa nessas 8 como a "memória" do jogo — juntas, elas descrevem exatamente
o que está acontecendo agora. Toda função do resto do arquivo faz uma coisa:
lê essas variáveis, ou muda elas, ou os dois.

| Variável | O que guarda | Nasce como |
|---|---|---|
| `cubo` | o cubo de verdade da partida | resolvido (chama `cuboResolvido()`) |
| `historico` | lista dos movimentos já feitos | vazia |
| `semente` | número usado pra embaralhar | `2024` |
| `tamanhoEmbaralho` | quantos movimentos embaralhar | `9` |
| `mensagem` | texto mostrado embaixo na tela | o texto inicial |
| `temSolucao` | "existe solução calculada válida pro cubo AGORA?" | `false` |
| `ultimaBusca` | resultado completo da última busca (se achou, quais passos, quantos estados visitou...) | vazio (o próprio tipo `Resultado` já nasce assim) |
| `modoIsometrico` | qual desenho do cubo mostrar | `false` (planificação) |

## 1.4 — `limparTela()` (linha 26)

```cpp
static void limparTela() { std::printf("\x1b[2J\x1b[H"); }
```

Não desenha nada — manda um **código de comando** pro terminal (não é
texto pra você ler). `\x1b` é o caractere ESC, que avisa o terminal "o que
vem a seguir é uma instrução". `[2J` = apaga a tela inteira. `[H` = volta o
cursor pro canto superior esquerdo. Resultado: tela em branco, pronta pra
desenhar de novo por cima.

Chamada em 3 lugares: dentro de `desenharTela()` (é por isso que a tela
"pisca" a cada tecla), dentro de `compararTodas()` e dentro do `main()`
(no final, antes do "Até mais!").

## 1.5 — `teclasDoMovimento()` (linhas 28-38)

```cpp
static std::string teclasDoMovimento(int mov)
{
    static const char LETRA[3] = {'u', 'r', 'f'};
    int face = mov / 3, variacao = mov % 3;
    char horario = LETRA[face];
    char antiHorario = (char) toupper(horario);

    if (variacao == 0) return std::string(1, horario);
    if (variacao == 2) return std::string(1, antiHorario);
    return std::string(1, horario) + " " + std::string(1, horario);
}
```

Recebe um **código de movimento** (0 a 8 — lembra a tabela dos 9
movimentos: `U U2 U' R R2 R' F F2 F'`) e devolve **qual tecla** você
apertaria na mão pra fazer esse mesmo giro.

- `face = mov / 3` → escolhe a letra certa na lista `LETRA` (`u`, `r` ou `f`).
- `variacao = mov % 3` → `0` = horário (letra minúscula), `2` = anti-horário
  (maiúscula, via `toupper`), e o que sobra (`1`, que é 180°) vira a letra
  **repetida duas vezes** (`"u u"`) — porque girar 180° é fisicamente o
  mesmo que girar 90° duas vezes seguidas.

Fica inteira dentro do `main.cpp` — não entra em nenhum outro arquivo, só
usa `toupper`, que é uma função pronta do C++.

**Exemplo:** `teclasDoMovimento(8)`. `face = 8/3 = 2` → `'f'`.
`variacao = 8%3 = 2` → devolve `antiHorario` = `toupper('f')` = `'F'`.

## 1.6 — `nomesDosPassos()` (linhas 40-48)

```cpp
static std::string nomesDosPassos(const std::vector<int> &passos)
{
    std::string s;
    for (size_t i = 0; i < passos.size(); i++) {
        if (i) s += "  ";
        s += teclasDoMovimento(passos[i]);
    }
    return s.empty() ? "(nenhum - ja estava resolvido)" : s;
}
```

Recebe uma **lista inteira** de movimentos (por exemplo, a solução achada
pela IA) e transforma numa única linha de texto, chamando
`teclasDoMovimento()` pra cada item da lista, um de cada vez, colando tudo
com dois espaços entre os movimentos. Se a lista vier vazia (cubo já
resolvido, zero passos necessários), devolve um aviso em vez de texto vazio.

## 1.7 — `desenharTela()` (linhas 50-90)

Essa função só **lê** o estado (as 8 variáveis da seção 1.3) e imprime na
tela o que combina com ele. Ela não decide nada, só mostra.

```cpp
static void desenharTela()
{
    limparTela();
    std::printf("  =============== CUBO MAGICO 2x2x2 ===============\n");
    if (modoIsometrico) imprimirCuboIso(cubo);
    else                imprimirCubo(cubo);
```

Primeiro apaga a tela, imprime o título fixo, e decide **qual desenho do
cubo** mostrar — se `modoIsometrico` estiver ligado, chama
`imprimirCuboIso()`; senão, `imprimirCubo()`. As duas moram no `Cubo.cpp` —
vamos ver elas na Parte 2.

```cpp
    std::printf("   Movimentos feitos ... %zu\n", historico.size());
    std::printf("   Semente .............. %u  (%d movimentos ao embaralhar)\n",
                semente, tamanhoEmbaralho);
    std::printf("   Situacao ............. %s\n\n",
                estaResolvido(cubo) ? "RESOLVIDO" : "embaralhado");
```

Mostra quantos movimentos você já fez (`historico.size()` — tamanho da
lista), a semente e o tamanho do embaralhamento, e se o cubo tá resolvido
(pergunta pra `estaResolvido()`, que também mora no `Cubo.cpp`).

```cpp
    if (temSolucao) {
        if (ultimaBusca.encontrou) {
            std::printf("   Ultima busca: %zu movimento(s), %lu estados visitados, %.3fs\n",
                        ultimaBusca.passos.size(), ultimaBusca.visitados, ultimaBusca.segundos);
            std::printf("   Solucao (teclas, na ordem): %s\n\n", nomesDosPassos(ultimaBusca.passos).c_str());
        } else {
            std::printf("   Ultima busca: sem solucao dentro do limite testado.\n\n");
        }
    }
```

Esse bloco **só aparece** se `temSolucao` for `true` — ou seja, se você já
rodou uma busca (`1`, `2`, `3` ou `m`). Se você abriu o programa agora e
nunca apertou nenhuma dessas, essa parte inteira fica pulada. Dentro dele,
mostra quantos movimentos a solução tem, quantos estados a busca visitou, o
tempo, e a lista de teclas (via `nomesDosPassos()`, que acabamos de ver).

```cpp
    std::printf("   ---------------------------------------------------------------\n");
    std::printf("    %-12s %-9s %s\n", "JOGAR",      "u r f",   "giro horario");
    ...
    std::printf("   >> %s\n", mensagem.c_str());
}
```

O resto (linhas 73-89) é só o menu de teclas, sempre igual — cada `printf`
imprime uma linha, com `%-12s %-9s %s` alinhando as 3 colunas (categoria,
tecla, descrição). A última linha imprime a `mensagem` — o texto que cada
ação (girar, embaralhar...) deixou pra você ler.

## 1.8 — `jogar(mov)` (linhas 92-98) — aplica um movimento manual

```cpp
static void jogar(int mov)
{
    cubo = mover(cubo, mov);
    historico.push_back(mov);
    temSolucao = false;
    mensagem = "Movimento '" + teclasDoMovimento(mov) + "' aplicado.";
}
```

Chamada pelas teclas `u r f U R F`, cada uma passando um código de
movimento diferente (0, 6, 3, 2, 8, 5... conferir na seção 1.15).

1. `cubo = mover(cubo, mov);` — **entra no `Cubo.cpp`**, gira o cubo de
   verdade (ver Parte 2). O resultado substitui a variável `cubo`.
2. `historico.push_back(mov);` — adiciona esse movimento no final da lista.
3. `temSolucao = false;` — qualquer solução calculada antes não serve mais,
   porque o cubo mudou.
4. Monta a mensagem de confirmação, usando `teclasDoMovimento()` (seção
   1.5) pra mostrar qual tecla foi.

## 1.9 — `desfazer()` (linhas 100-108) — tecla `z`

```cpp
static void desfazer()
{
    if (historico.empty()) { mensagem = "Nada para desfazer."; return; }
    int ultimo = historico.back();
    historico.pop_back();
    cubo = mover(cubo, movimentoInverso(ultimo));
    temSolucao = false;
    mensagem = "Ultimo movimento desfeito.";
}
```

1. Se a lista `historico` estiver vazia, não tem nada pra desfazer — avisa
   e sai (`return`) sem fazer mais nada.
2. `historico.back()` olha o **último** movimento feito, sem tirar ele
   ainda. `historico.pop_back()` tira esse último item da lista.
3. `movimentoInverso(ultimo)` — **entra no `Cubo.cpp`** — descobre qual
   movimento desfaz o `ultimo` (por exemplo, o inverso de `R` é `R'`).
4. Aplica esse movimento inverso no cubo (de novo, via `mover()`, no
   `Cubo.cpp`).
5. Mesma lógica de sempre: `temSolucao = false`, porque o cubo mudou.

## 1.10 — `resetarPrograma()` (linhas 110-120) — tecla `C`

```cpp
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
```

Devolve **cada uma** das 8 variáveis de estado (seção 1.3) pro valor que
tinham quando o programa abriu — é o "reset de fábrica" completo. Repara
que é literalmente uma linha por variável, na mesma ordem que elas foram
declaradas lá em cima.

## 1.11 — `embaralharAgora()` (linhas 122-139) — tecla `e`

```cpp
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
```

Diferente do resto do programa (que lê uma tecla só, sem Enter), essa
função pede pra você **digitar um número e apertar Enter** (`std::cin >>`).

1. Pergunta a semente; se você digitar um número válido, guarda em `semente`.
2. Pergunta quantos movimentos (só aceita de 1 a 40); se válido, guarda em
   `tamanhoEmbaralho`.
3. `embaralhar(cubo, semente, tamanhoEmbaralho);` — **entra no `Cubo.cpp`**
   — é aqui que o embaralhamento de verdade acontece (ver Parte 2).
4. Esvazia o histórico e desliga `temSolucao` (cubo novo, começando do
   zero).

## 1.12 — `resolver(estrategia)` (linhas 141-165) — teclas `1` `2` `3`

```cpp
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
    ...
    if      (estrategia == 0) ultimaBusca = buscaEmLargura(cubo);
    else if (estrategia == 1) ultimaBusca = buscaProfundidadeIterativa(cubo, 11);
    else                      ultimaBusca = buscaAEstrela(cubo);

    temSolucao = true;
    mensagem = ultimaBusca.encontrou ? "..." : "...";
}
```

`estrategia` chega valendo `0`, `1` ou `2` (dependendo de qual tecla — ver
seção 1.15).

1. Se o cubo já tá resolvido, nem tenta — avisa e sai.
2. Imprime "Rodando..." (a busca pode demorar, principalmente a de Largura).
3. **Aqui é a primeira vez que o `main.cpp` entra no `Busca.cpp`** — chama
   uma das três funções (`buscaEmLargura`, `buscaProfundidadeIterativa` ou
   `buscaAEstrela`, dependendo de `estrategia`), passando o `cubo` atual.
   O `11` no meio da chamada da profundidade iterativa é o limite máximo de
   profundidade que ela vai tentar (11 é o "diâmetro" do espaço de estados
   do 2x2x2 — nenhum cubo precisa de mais de 11 movimentos pra resolver).
   Ver a Parte 3 pra entender o que acontece dentro dessas três funções.
4. O que qualquer uma delas devolver (um `Resultado`) vira `ultimaBusca`.
5. `temSolucao = true` — agora sim existe uma solução válida guardada.

## 1.13 — `compararTodas()` (linhas 167-222) — tecla `m`

Mesma ideia de `resolver()`, só que roda **as três buscas em sequência**
no mesmo cubo, guarda os três resultados num vetor `r[3]`, e monta uma
tabela comparando quantos estados cada uma visitou, quantos gerou, o
tamanho da solução e o tempo. No final, calcula quantas vezes o A\* visitou
menos estados que as outras duas, espera você apertar uma tecla
(`lerTecla()` — ver Parte 4), e guarda o resultado do A\* como a "última
busca" (pronta pra aplicar com `a`).

## 1.14 — `aplicarSolucao()` (linhas 224-241) — tecla `a`

```cpp
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
        std::printf("...");
        lerTecla();
    }
    temSolucao = false;
    mensagem = "Solucao aplicada - o cubo esta resolvido.";
}
```

1. Primeiro checa se existe uma solução válida (`temSolucao` ligado e a
   busca tendo encontrado algo). Se não, avisa e sai — é essa checagem que
   impede de aplicar uma solução velha/inválida.
2. O `for (int mov : ultimaBusca.passos)` passa por **cada movimento da
   lista de passos**, um de cada vez: aplica no cubo (`mover()`, no
   `Cubo.cpp`), adiciona no histórico, redesenha a tela, mostra qual
   movimento foi (em tecla e em notação clássica), e espera você apertar
   qualquer tecla pra ver o próximo passo.
3. No final, desliga `temSolucao` — a solução já foi usada.

## 1.15 — `tratarTecla(tecla)` (linhas 243-291) — o roteador

```cpp
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
        mensagem = modoIsometrico ? "..." : "...";
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
```

Recebe `tecla` (o código da tecla que você apertou) e decide o que fazer.
**Cada tecla cai em exatamente um `case`**, nunca dois. Mapa completo:

| Tecla | Código do movimento (se aplicável) | O que chama |
|---|---|---|
| `u` | 0 (U horário) | `jogar(0)` |
| `U` | 2 (U anti-horário) | `jogar(2)` |
| `r` | 3 (R horário) | `jogar(3)` |
| `R` | 5 (R anti-horário) | `jogar(5)` |
| `f` | 6 (F horário) | `jogar(6)` |
| `F` | 8 (F anti-horário) | `jogar(8)` |
| `z` | — | `desfazer()` |
| `e` | — | `embaralharAgora()` |
| `v` | — | só inverte `modoIsometrico` (não chama função de fora) |
| `j` | — | `abrirJanela3D(...)` (só existe se compilado com `make 3d`) |
| `c` | — | volta o cubo pro resolvido, na mão (sem chamar `resetarPrograma`) |
| `C` | — | `resetarPrograma()` |
| `1` `2` `3` | — | `resolver(0/1/2)` |
| `m` | — | `compararTodas()` |
| `a` | — | `aplicarSolucao()` |
| `q` | — | devolve `false` (é o único `case` que faz isso) |
| qualquer outra | — | `default:` não faz nada, devolve `true` |

**Por que isso importa pro `while` do `main()`:** o valor que `tratarTecla`
devolve (`true` ou `false`) vira o novo valor de `rodando`. Só o `q` devolve
`false` — por isso é a única tecla que consegue fechar o programa.

**Números como código de tecla, ligando com os 9 movimentos:** repara que
os 6 primeiros `case`s chamam `jogar()` com os códigos `0, 2, 3, 5, 6, 8` —
faltam de propósito o `1`, `4` e `7` (os movimentos de 180°: `U2`, `R2`,
`F2`). Não tem tecla direta pra eles porque girar 180° é o mesmo que
apertar a tecla horária duas vezes seguidas — não faz falta.

## 1.16 — `main()` (linhas 293-307) — de novo, agora com tudo isso no bolso

```cpp
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
```

Agora que já vimos cada peça, o `main()` inteiro é só isto: liga as cores
(uma vez), e repete pra sempre **desenha → espera tecla → decide o que
fazer** — até `tratarTecla` devolver `false`.

---

# PARTE 2 — `Cubo.cpp` / `Cubo.hpp`: o motor do cubo

Essa parte é chamada pelo `main.cpp` toda hora (`mover`, `embaralhar`,
`cuboResolvido`, `estaResolvido`...). Ela **não sabe nada** sobre teclado,
tela ou IA — só entende de cubo.

## 2.1 — O `struct Cubo` (Cubo.hpp, linhas 13-16)

```cpp
struct Cubo {
    std::array<uint8_t, N_CANTOS> cp;
    std::array<uint8_t, N_CANTOS> co;
};
```

`cp` e `co` são **listas de 8 números** cada (`N_CANTOS` é 8). Pensa nelas
como 8 caixinhas numeradas de 0 a 7 — cada caixinha é uma posição física do
cubo (uma "casa" de canto).

- `cp[i]` = **qual peça** está sentada na casa `i` (a peça em si também é
  numerada 0-7).
- `co[i]` = **quanto essa peça está torcida** (0, 1 ou 2).

Cubo resolvido = cada peça na sua própria casa (`cp[i] == i` pra todo `i`) e
sem torção (`co[i] == 0` pra todo `i`).

**Numeração das casas** (comentário original do código):
```
0 = URF   1 = UFL   2 = ULB   3 = UBR
4 = DFR   5 = DLF   6 = DRB   7 = DBL  (fixa)
```

**Por que a peça DBL (casa 7) nunca se move:** o 2x2x2 não tem peça de
centro, então girar o cubo inteiro na mão não muda o quebra-cabeça — só
muda de que ângulo você olha. Se o código não fizesse nada a respeito
disso, cada configuração física teria 24 jeitos diferentes de ser
representada (uma pra cada rotação do cubo inteiro), o que complicaria
tudo. A solução: **trava-se uma peça de referência** (a DBL) e só se gira as
3 faces que nunca a tocam (U, R, F). Isso ainda alcança qualquer
configuração possível, só que agora cada uma tem **um único** jeito de
aparecer no código.

## 2.2 — `cuboResolvido()` e `estaResolvido()` (linhas 24-36)

```cpp
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
```

`cuboResolvido()`: cria uma variável `c` **nova e vazia**, do tipo `Cubo`
(ela só existe enquanto essa função roda — assim que a função termina e
devolve o resultado com `return c;`, essa cópia específica "some", só
sobrevive o valor que foi entregue). Preenche as 8 casas, cada uma com a
peça de mesmo número e sem torção. Isso é a FUNÇÃO AVALIADORA de base —
"como é o objetivo".

`estaResolvido()`: passa pelas 8 casas; se **qualquer uma** tiver a peça
errada OU estiver torcida, devolve `false` na hora. Só devolve `true` se
passar pelas 8 sem achar problema. Essa é a **função avaliadora de verdade**
usada pela busca (via `ehObjetivo()`, no `Busca.cpp` — ver Parte 3).

## 2.3 — As tabelas `PERM` e `TWIST` (linhas 13-22)

```cpp
static const uint8_t PERM[3][N_CANTOS] = {
     {3, 0, 1, 2, 4, 5, 6, 7},   // face U
     {4, 1, 2, 0, 6, 5, 3, 7},   // face R
     {1, 5, 2, 3, 0, 4, 6, 7}    // face F
};
static const uint8_t TWIST[3][N_CANTOS] = {
     {0, 0, 0, 0, 0, 0, 0, 0},   // face U
     {2, 0, 0, 1, 1, 0, 2, 0},   // face R
     {1, 2, 0, 0, 2, 1, 0, 0}    // face F
};
```

`[3][N_CANTOS]` é uma tabela de 3 linhas (uma por face: U, R, F) e 8
colunas (uma por casa). Essas tabelas foram escritas **na mão, uma vez só**,
olhando o cubo de verdade — depois disso, o código nunca mais precisa pensar
em geometria, só consulta.

**A pegadinha de como ler a tabela:** `PERM[face][i]` não diz "pra onde a
peça da casa `i` foi" — diz o **contrário**: "de qual casa **veio** a peça
que agora está na casa `i`". Exemplo de aplicar um giro de U num cubo
resolvido (onde cada casa `i` tinha a peça `i`):

| Casa nova `i` | `PERM[U][i]` (de onde veio) | Resultado |
|---|---|---|
| 0 | 3 | a peça que estava na casa 3 agora está na 0 |
| 1 | 0 | a peça que estava na casa 0 agora está na 1 |
| 2 | 1 | a peça que estava na casa 1 agora está na 2 |
| 3 | 2 | a peça que estava na casa 2 agora está na 3 |
| 4, 5, 6, 7 | 4, 5, 6, 7 (cada uma pra ela mesma) | ninguém trocou de lugar |

As casas 4-7 (a camada de baixo) nem mudam — faz sentido, girar U só mexe
nas 4 peças de cima.

`TWIST[face][i]` guarda **quanto a peça se torce** ao chegar na casa `i`
através daquela face (0, 1 ou 2 "giros de 120 graus"). Repara que a linha
de U é toda zero — girar U não torce ninguém. Já R e F torcem algumas
peças, porque geometricamente elas "empurram" a peça pra um ângulo
diferente.

## 2.4 — `umQuartoDeVolta()` (linhas 38-47) — gira 90°, usando as tabelas

```cpp
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
```

Pensa em `o` como uma **foto do cubo antes de girar**, e `d` como a **foto
de depois** que essa função está montando, casa por casa. A função nunca
muda a foto de antes — só lê ela, e desenha a foto nova do zero.

Pra cada casa `i`: descobre de onde a peça que vai ficar ali **veio**
(`origem = PERM[face][i]`), copia a identidade dessa peça (`o.cp[origem]`)
pra casa nova, e calcula a nova torção: pega a torção que a peça já tinha
(`o.co[origem]`), **soma** a torção que esse giro aplica (`TWIST[face][i]`),
e usa `% 3` (resto da divisão por 3) pra "dar a volta" se passar de 2 —
torção só tem 3 valores possíveis (0, 1, 2), então é tipo um relógio de 3
horas.

**Exemplo numérico, aplicando R num cubo resolvido** (`o.cp = [0..7]`,
`o.co = [0,0,0,0,0,0,0,0]`), usando `PERM[R] = {4,1,2,0,6,5,3,7}` e
`TWIST[R] = {2,0,0,1,1,0,2,0}`:

| `i` | `origem=PERM[R][i]` | `d.cp[i]=o.cp[origem]` | `d.co[i]=(0+TWIST[R][i])%3` |
|---|---|---|---|
| 0 | 4 | 4 | `(0+2)%3 = 2` |
| 3 | 0 | 0 | `(0+1)%3 = 1` |
| 4 | 6 | 6 | `(0+1)%3 = 1` |
| 6 | 3 | 3 | `(0+2)%3 = 2` |

(as casas 1, 2, 5, 7 não mudam, porque R não as toca)

## 2.5 — `mover()` (linhas 49-57) — a FUNÇÃO SUCESSORA

```cpp
Cubo mover(const Cubo &origem, int movimento)
{
    int face  = movimento / 3;
    int vezes = movimento % 3 + 1;
    Cubo atual = origem;
    for (int k = 0; k < vezes; k++)
        atual = umQuartoDeVolta(atual, face);
    return atual;
}
```

Essa é a função que **todo o resto do programa** usa pra girar o cubo —
recebe um cubo e um código de movimento (0-8), devolve o cubo depois do
movimento.

- `movimento / 3` (divisão inteira, despreza o resto) → recupera a **face**.
  Exemplo: movimento 7 → `7/3 = 2` → face F.
- `movimento % 3` (resto da divisão) → a **variação**, e soma `+1` pra virar
  "quantos quartos de volta": variação 0 → 1 quarto; variação 1 (180°) → 2
  quartos; variação 2 (anti-horário) → 3 quartos (3 giros de 90° no sentido
  horário dá o mesmo resultado físico que 1 giro anti-horário).
- O `for` chama `umQuartoDeVolta()` repetidamente, tantas vezes quanto
  `vezes` — cada chamada usa o resultado da anterior como ponto de partida.

**Sacada importante:** o código só tem tabela pronta pra 90° horário (3
faces). Os outros 6 movimentos (180° e anti-horário) nascem de **repetir**
essa mesma função, sem tabela nova nenhuma.

## 2.6 — `indiceDoEstado()` (linhas 59-71) — transforma um cubo num número único

```cpp
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
```

**Pra que serve:** a busca (Parte 3) precisa marcar "já visitei esse
estado" rapidamente, em vez de comparar cubo por cubo um a um (o que seria
muito lento com milhões de estados). A solução: transformar cada cubo
possível num número único, entre 0 e 3.674.159, e usar esse número como
índice de um vetor gigante de "já visitei ou não".

Não precisa decorar a matemática exata (é um "código de Lehmer", um jeito
clássico de numerar permutações), mas a ideia central: a primeira metade da
função (`perm`) numera **a ordem das peças** (qual arranjo, entre os `7! =
5040` possíveis), e a segunda metade (`orient`) numera **as torções** (lendo
`co[0]` a `co[5]` como um número em base 3, de 0 a 728 — a 7ª torção não
entra porque ela é sempre determinada pelas outras 6). No final,
`perm * 729 + orient` junta as duas partes num único número, sem nenhum
número repetido pra cubos diferentes.

## 2.7 — `embaralhar()` (linhas 73-99) — bagunça o cubo de forma reproduzível

```cpp
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
```

`proximoAleatorio()` é um **gerador de números "aleatórios" próprio**
(técnica chamada xorshift32 — mistura os bits do número com deslocamentos e
"ou exclusivo"). Não usa a função `rand()` pronta do C++ porque o resultado
dela varia de compilador pra compilador — com esse gerador próprio, a mesma
semente sempre dá o mesmo resultado, em qualquer máquina.

`embaralhar()`:
1. Sempre começa do cubo **resolvido** (`c = cuboResolvido()`), pra o
   resultado depender só da semente, nunca do estado anterior do cubo.
2. Pra cada um dos `n` movimentos pedidos: sorteia uma face (0, 1 ou 2)
   **diferente da última** (o `do...while` repete o sorteio até dar uma
   face diferente — evita desperdiçar movimentos girando a mesma face duas
   vezes seguidas), sorteia a variação, monta o código do movimento, aplica
   no cubo (via `mover()`) e guarda na lista `movimentos` (que a função
   devolve no final).

## 2.8 — `movimentoPorNome()`, `mesmaFace()`, `movimentoInverso()` (linhas 101-127)

Três funções pequenas de apoio:

- `movimentoPorNome("R'")` → devolve o código `5` (faz o caminho inverso de
  `NOME_MOVIMENTO`, que converte texto pra código).
- `mesmaFace(movA, movB)` → `movA / 3 == movB / 3` — verdadeiro se os dois
  movimentos giram a mesma face (independente da variação). Usado na busca
  pra evitar girar a mesma face duas vezes seguidas (desperdício).
- `movimentoInverso(mov)` → devolve o movimento que desfaz `mov`. Se a
  variação for 180° (`v==1`), o inverso é ele mesmo. Senão, troca horário
  por anti-horário (variação 0 vira 2, e vice-versa).

## 2.9 — Visualização: `facelets()`, `imprimirCubo()`, `imprimirCuboIso()`, `habilitarCoresNoTerminal()`

Essa parte **não faz parte da lógica do quebra-cabeça** — é só sobre
desenhar o cubo na tela. Resumo rápido:

- `CANTO_FACELET[i][k]` (tabela, linhas 129-138): diz qual das 24
  "figurinhas" (adesivos visíveis) corresponde ao `k`-ésimo lado visível da
  casa `i`.
- `facelets(c)` (linhas 145-154): converte o estado (`cp`/`co`) nas 24 cores
  visíveis, usando `CANTO_FACELET` e uma tabela de cores por peça
  (`COR_PECA`).
- `imprimirCubo()` (linhas 171-200): desenha a planificação completa (6
  faces), usando `facelets()`.
- `imprimirCuboIso()` (linhas 202-224): desenha só 3 faces (U, F, R), com um
  leve deslocamento de posição pra sugerir profundidade — não é uma câmera
  3D de verdade, só um efeito visual barato.
- `habilitarCoresNoTerminal()` (linhas 226-234): liga o suporte a cores
  ANSI no console do Windows (conversamos sobre ela lá na Parte 1).

---

# PARTE 3 — `Fronteira.hpp` + `Busca.cpp`: a Inteligência Artificial

Essa é a parte que resolve o cubo sozinha. O `main.cpp` só chama uma das
três funções públicas (`buscaEmLargura`, `buscaAEstrela`,
`buscaProfundidadeIterativa`) — tudo o que acontece por dentro fica
escondido aqui.

## 3.1 — `Fronteira.hpp`: as 3 estruturas de dados

O enunciado do trabalho pede que as três buscas usem **o mesmo laço**,
trocando só a estrutura de dados que guarda os estados esperando pra ser
examinados. Por isso existe essa "interface comum":

```cpp
class Fronteira {
public:
    virtual void inserir(int idNo, int chave) = 0;
    virtual int  remover() = 0;
    virtual bool vazia() const = 0;
};
```

Isso declara que **qualquer** `Fronteira` tem que saber fazer 3 coisas:
`inserir` um item, `remover` o próximo item (segundo a própria regra dela),
e dizer se está `vazia`. O `= 0` quer dizer que essa classe não tem código
próprio pra essas funções — quem tem são as 3 classes que "herdam" dela:

**`FilaFronteira`** (FIFO — primeiro que entra, primeiro que sai):
```cpp
void inserir(int idNo, int) override { d.push_back(idNo); }
int  remover() override { int n = d.front(); d.pop_front(); return n; }
```
Insere no fim, remove do início. Usada pela **Busca em Largura**: como
sempre tira quem está esperando há mais tempo, ela examina os estados
"camada por camada" (todos com 1 movimento antes de qualquer um com 2).

**`PilhaFronteira`** (LIFO — último que entra, primeiro que sai):
```cpp
void inserir(int idNo, int) override { v.push_back(idNo); }
int  remover() override { int n = v.back(); v.pop_back(); return n; }
```
Insere no topo, remove do topo. Usada pela **Busca em Profundidade
Limitada**: como sempre continua pelo caminho mais recente, ela "mergulha"
fundo por um caminho antes de voltar pra tentar outro.

**`PrioridadeFronteira`** (heap — sempre tira quem tem a menor "chave"):
```cpp
void inserir(int idNo, int chave) override {
    dados.push_back({idNo, chave, contador++});
    std::push_heap(dados.begin(), dados.end(), Comparador());
}
int remover() override {
    std::pop_heap(dados.begin(), dados.end(), Comparador());
    ...
}
```
Usa um **heap binário** (uma estrutura organizada pra sempre achar o menor
valor rapidamente). Usada pelo **A\***: a "chave" de cada item é o quanto
aquele caminho parece promissor (quanto menor, melhor), então ela sempre
examina primeiro o estado que parece mais perto da solução.

## 3.2 — `Busca.hpp`: o que uma busca devolve

```cpp
struct Resultado {
    bool encontrou = false;
    std::vector<int> passos;
    unsigned long visitados = 0;
    unsigned long gerados   = 0;
    double segundos = 0.0;
    int limiteFinal = -1;
};
```

`encontrou`: achou solução ou não. `passos`: a lista de movimentos da
solução, em ordem. `visitados`: quantos estados foram **retirados** da
estrutura (examinados de verdade). `gerados`: quantos foram **colocados**
na estrutura (nem todos acabam sendo examinados). `segundos`: tempo gasto.

Repara que cada campo já tem um valor padrão (`= false`, `= 0`...) — é por
isso que, lá no `main.cpp`, `static Resultado ultimaBusca;` já nasce
"vazia" sem precisar escrever nada mais.

## 3.3 — `No` e `MemoriaDaBusca` (Busca.cpp, linhas 6-46)

```cpp
struct No {
    Cubo cubo;
    int pai;
    uint8_t movimento;
    uint8_t profundidade;
};
```

Cada `No` representa um estado **dentro da árvore de busca** — guarda o
cubo daquele estado, qual foi o "pai" dele (o nó anterior, de onde ele
veio) e qual movimento levou até ali. É assim que, no final, dá pra
**reconstruir o caminho completo** da solução: começa no nó objetivo e vai
voltando pelos pais até chegar na raiz.

```cpp
struct MemoriaDaBusca {
    std::vector<No> pool;
    std::vector<uint8_t> menorProfundidade;
    std::vector<int> noDoEstado;
    ...
```

`pool`: guarda **todos** os nós já criados nessa busca (o "id" de um nó é
só a posição dele nessa lista). `menorProfundidade`: um vetor com um item
pra **cada estado possível** (lembra, `N_ESTADOS` = 3.674.160), guardando a
menor profundidade em que aquele estado já foi visto — serve pra saber se
vale a pena "reabrir" um estado por um caminho mais curto. `noDoEstado`:
outro vetor do mesmo tamanho, dizendo qual nó (posição no `pool`)
representa cada estado — assim, cada estado tem **no máximo um** nó vivo,
nunca duplicado.

`reiniciar()`: limpa tudo de novo (usado entre as rodadas da Profundidade
Iterativa, que roda o laço várias vezes). `novoNo(...)`: cria um nó e
devolve a posição dele no `pool`. `reconstruirCaminho(...)`: anda dos pais
até a raiz, juntando os movimentos (de trás pra frente, depois inverte a
ordem) — monta a lista final de `passos`.

## 3.4 — `ehObjetivo()` e `heuristica()` (linhas 48-56)

```cpp
bool ehObjetivo(const Cubo &c) { return estaResolvido(c); }
```
A FUNÇÃO AVALIADORA de verdade — só chama `estaResolvido()`, que já vimos
no `Cubo.cpp`.

```cpp
int heuristica(const Cubo &c)
{
    int fora = 0;
    for (int i = 0; i < 7; i++)
        if (c.cp[i] != i || c.co[i] != 0) fora++;
    return (fora + 3) / 4;
}
```

A heurística é uma **estimativa** de quantos movimentos ainda faltam —
usada só pelo A\*, pra ele saber qual caminho parece mais promissor. Conta
quantos dos 7 cantos móveis estão fora do lugar ou torcidos (`fora`), e
devolve `⌈fora/4⌉` (teto da divisão por 4 — `(fora+3)/4` é um jeito de
calcular teto usando divisão inteira).

**Por que dividir por 4:** cada um dos 9 movimentos mexe em **exatamente 4**
das 7 posições de canto móveis (nenhum movimento toca a peça fixa DBL).
Então um único movimento só consegue "acertar" no máximo 4 peças de uma
vez — se há `fora` peças erradas, são necessários pelo menos `⌈fora/4⌉`
movimentos. Como isso nunca **superestima** o custo real, a heurística é
**admissível** — e como o valor de `fora` muda no máximo 4 por movimento
(logo `⌈fora/4⌉` muda no máximo 1), ela também é **consistente**. Isso
garante que o A\* sempre acha a solução **ótima** (o menor número de
movimentos possível).

## 3.5 — `lacoDeBusca()` (linhas 58-107) — o coração do trabalho

Esse é **o** laço exigido pelo enunciado — o mesmo código, sem mudar uma
linha, serve pras três estratégias. O que muda é só qual `Fronteira`
concreta é passada.

```cpp
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    mem.menorProfundidade[indiceDoEstado(inicial)] = 0;
    mem.noDoEstado[indiceDoEstado(inicial)] = raiz;
    fr.inserir(raiz, heuristica(inicial));
    r.gerados++;
```

**1. Adicionar estado inicial:** cria o nó raiz (sem pai, profundidade 0),
marca ele como visto (`menorProfundidade` e `noDoEstado`), e coloca na
estrutura (`fr.inserir`).

```cpp
    while (!fr.vazia()) {
        int atual = fr.remover();
        Cubo estado = mem.pool[(size_t) atual].cubo;
        int prof    = mem.pool[(size_t) atual].profundidade;
        r.visitados++;

        if (ehObjetivo(estado)) {
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }
```

**2. Enquanto a estrutura não estiver vazia:** tira o próximo estado
(`fr.remover()` — quem sai primeiro depende de qual `Fronteira` foi
passada: fila, pilha ou heap). Conta como visitado. **2.2 Avaliar estado:**
se for o objetivo, reconstrói o caminho até ele e termina a função ali
mesmo (`return`).

```cpp
        if (limite >= 0 && prof >= limite) continue;

        for (int m = 0; m < N_MOVIMENTOS; m++) {
            if (mem.pool[(size_t) atual].pai >= 0 &&
                mesmaFace(m, mem.pool[(size_t) atual].movimento))
                continue;

            Cubo vizinho = mover(estado, m);
            uint32_t idx = indiceDoEstado(vizinho);

            if (mem.menorProfundidade[idx] <= prof + 1) continue;
            mem.menorProfundidade[idx] = (uint8_t) (prof + 1);

            int filho = mem.noDoEstado[idx];
            if (filho >= 0) {
                mem.pool[(size_t) filho].pai          = atual;
                mem.pool[(size_t) filho].movimento    = (uint8_t) m;
                mem.pool[(size_t) filho].profundidade = (uint8_t) (prof + 1);
            } else {
                filho = mem.novoNo(vizinho, atual, m, prof + 1);
                mem.noDoEstado[idx] = filho;
            }
            fr.inserir(filho, (prof + 1) + heuristica(vizinho));
            r.gerados++;
        }
    }

    r.encontrou = false;
}
```

**2.3 Adicionar estados seguintes:** se já bateu no limite de profundidade
(só relevante pra Profundidade Limitada), não expande mais esse estado
(`continue`, pula pro próximo do `while`). Senão, testa os 9 movimentos
possíveis:

- Pula se for girar a mesma face que o movimento anterior (desperdício —
  `mesmaFace`).
- Calcula o vizinho (`mover()`) e o índice dele (`indiceDoEstado()`).
- Se esse estado já foi visto por um caminho **igual ou mais curto**, pula
  (`menorProfundidade[idx] <= prof + 1`). Senão, atualiza a menor
  profundidade conhecida.
- Se o estado já tinha um nó (`filho >= 0`), reaproveita ele, só atualizando
  pai/movimento/profundidade pro caminho novo (mais curto). Senão, cria um
  nó novo.
- Coloca na estrutura, com chave `(prof+1) + heuristica(vizinho)` — o
  `g + h` clássico (`g` = quantos movimentos até aqui, `h` = estimativa do
  que falta). A fila e a pilha **ignoram** essa chave (olha o `Fronteira.hpp`
  de novo — o parâmetro `chave` delas nem tem nome, só a fila de
  prioridade usa de verdade).

**3. Se a estrutura esvaziar sem achar o objetivo:** `r.encontrou = false`.

## 3.6 — As três buscas, uma por uma: o que cada uma É e como funciona

As três (linhas 109-161 do `Busca.cpp`) usam **o mesmo `lacoDeBusca()`** que
acabamos de ver na seção 3.5 — nenhuma delas tem um laço próprio. A
identidade de cada uma vem inteiramente de **qual `Fronteira`** (seção 3.1)
é entregue pro laço, e se existe ou não um limite de profundidade. É só
isso. Vamos ver cada uma separadamente: o que ela é como *ideia* de
inteligência artificial, e depois como essas poucas linhas de código
realmente implementam essa ideia.

### 3.6.1 — Busca em Largura (BFS — *Breadth-First Search*)

**O que é, como ideia:** imagina a água enchendo uma piscina a partir de um
ponto — ela se espalha **igualmente em todas as direções ao mesmo tempo**,
nunca avançando muito numa direção sem ter enchido tudo mais perto primeiro.
É exatamente isso: a Busca em Largura examina **todos** os estados
alcançáveis com 1 movimento, depois **todos** os alcançáveis com 2
movimentos, depois todos com 3... nunca pulando uma "camada" de
profundidade sem ter terminado a anterior.

**Por que isso garante a solução mais curta:** como ela só avança pra
camada `N+1` depois de esgotar **inteiramente** a camada `N`, o primeiro
estado-objetivo que ela encontrar necessariamente está na camada mais rasa
possível — não existe nenhum jeito mais curto de chegar lá, porque se
existisse, ele já teria sido achado numa camada anterior.

**Como o código faz isso, especificamente:**
```cpp
Resultado buscaEmLargura(const Cubo &inicial)
{
    Resultado r;
    MemoriaDaBusca mem;
    FilaFronteira fr;
    lacoDeBusca(fr, mem, inicial, -1, r);
    ...
    return r;
}
```
A única coisa "especial" aqui é passar uma `FilaFronteira` (fila FIFO —
primeiro que entra, primeiro que sai) pro laço, e `-1` no lugar do limite
(sem limite de profundidade). Como a fila sempre devolve o estado que está
esperando **há mais tempo**, e os estados são colocados na fila na ordem em
que são descobertos (primeiro os de profundidade 1, depois os de
profundidade 2 que nasceram deles, etc.), o próprio comportamento de "fila"
já garante o efeito de "camada por camada" — o código não precisa controlar
profundidade manualmente pra isso, é uma consequência natural de usar FIFO.

**Ponto fraco:** ela não usa nenhuma informação sobre "o quão perto" um
estado parece estar da solução — trata todo estado da mesma camada como
igualmente importante. Isso faz ela **visitar muitíssimos estados**
(cresce exponencialmente com a profundidade) antes de achar a solução,
mesmo garantindo que ela é a mais curta.

### 3.6.2 — Busca em Profundidade Limitada Iterativa (IDDFS)

**O que é, como ideia:** ao contrário da Largura, uma busca em profundidade
"pura" mergulha fundo por **um único caminho** até o fim antes de voltar e
tentar outro — tipo explorar um labirinto sempre virando à direita até
bater numa parede, só aí voltando pra tentar outro caminho. O problema de
fazer isso **sem limite**: num espaço com ciclos (como o do cubo, onde você
pode desfazer e refazer movimentos pra sempre), ela pode nunca terminar, ou
gastar um tempo gigantesco antes de voltar atrás.

A solução usada aqui tem duas palavras no nome, e as duas importam:
**Limitada** — a busca não pode descer além de um certo número de
movimentos (`limite`); ao bater nesse teto, ela para de expandir aquele
ramo e volta. **Iterativa** — em vez de escolher um limite fixo (que
poderia ser curto demais ou gastar memória à toa se fosse longo demais), o
código roda a busca **várias vezes**, aumentando o limite de 1 em 1 (0,
depois 1, depois 2...), até uma dessas rodadas encontrar a solução.

**Por que isso também garante a solução mais curta:** a rodada com
`limite = 0` só acha solução se o cubo já estiver resolvido. A rodada com
`limite = 1` só acha se resolver em exatamente 1 movimento (ou menos). E
assim por diante. A **primeira** rodada que encontra alguma solução, por
construção, não podia ter achado numa rodada de limite menor — então é a
mais curta possível.

**Como o código faz isso, especificamente:**
```cpp
Resultado buscaProfundidadeIterativa(const Cubo &inicial, int limiteMaximo)
{
    Resultado total;
    MemoriaDaBusca mem;

    for (int limite = 0; limite <= limiteMaximo; limite++) {
        Resultado r;
        PilhaFronteira fr;
        mem.reiniciar();
        lacoDeBusca(fr, mem, inicial, limite, r);

        total.visitados += r.visitados;
        total.gerados   += r.gerados;
        if (r.encontrou) { total.encontrou = true; total.passos = r.passos; break; }
    }
    return total;
}
```
O `for` externo é a parte "iterativa" — cada volta roda `lacoDeBusca()` do
zero (por isso `mem.reiniciar()` antes de cada uma), com um `limite`
maior. Dentro de cada rodada, quem dá o comportamento de "profundidade" é a
`PilhaFronteira` (pilha LIFO — último que entra, primeiro que sai): como
ela sempre continua pelo estado mais **recente**, o laço mergulha fundo por
um caminho antes de voltar pra tentar outro. E é o parâmetro `limite`
(repassado direto pro `lacoDeBusca`) que impede ela de mergulhar fundo
demais — olha de novo a linha `if (limite >= 0 && prof >= limite) continue;`
lá na seção 3.5.

`total.visitados`/`total.gerados` **somam o esforço de todas as rodadas**,
inclusive as que não acharam nada (linhas 0 até a penúltima) — é por isso
que essa estratégia normalmente reporta mais estados visitados que a Busca
em Largura, mesmo achando exatamente a mesma solução ótima: ela refaz o
trabalho das rodadas anteriores toda vez que aumenta o limite.

### 3.6.3 — A\* (A-estrela)

**O que é, como ideia:** as duas buscas anteriores são "cegas" — nenhuma
delas usa qualquer informação sobre o cubo além de "girei ou não girei".
O A\* é diferente: ele usa uma **heurística** (seção 3.4 — a estimativa de
quantos movimentos ainda faltam) pra decidir **qual estado examinar
primeiro**, entre todos os que estão esperando. Em vez de andar
"às cegas", ele sempre olha primeiro pro estado que **parece** mais
promissor.

A régua usada pra decidir "quão promissor" é a soma `f = g + h`:
- `g` = quantos movimentos já foram gastos pra chegar até aquele estado
  (o custo **real**, já conhecido).
- `h` = a heurística — quantos movimentos a mais **parecem** faltar (uma
  estimativa, um "chute educado").
- `f` = estimativa do custo **total** do caminho, se aquele estado for o
  escolhido.

A cada passo, o A\* expande o estado de **menor `f`** entre os que estão
esperando — a aposta mais barata de ponta a ponta.

**Por que ele ainda garante a solução ótima** (não é só "mais rápido, mas
arriscado"): como provado na seção 3.4, a heurística usada aqui nunca
superestima o custo real (é **admissível**) e muda no máximo 1 por
movimento (é **consistente**). Isso garante matematicamente que, quando o
A\* retira um estado da fronteira e ele é o objetivo, o caminho até ali já
é garantidamente o mais curto possível — sem precisar reabrir nenhum nó já
expandido.

**Como o código faz isso, especificamente:**
```cpp
Resultado buscaAEstrela(const Cubo &inicial)
{
    Resultado r;
    MemoriaDaBusca mem;
    PrioridadeFronteira fr;
    lacoDeBusca(fr, mem, inicial, -1, r);
    ...
    return r;
}
```
Só troca a `Fronteira` pra `PrioridadeFronteira` (o heap binário da seção
3.1) — sem limite de profundidade (`-1`), igual a Busca em Largura. A conta
do `f = g + h` nem aparece aqui: ela acontece lá dentro do `lacoDeBusca()`,
na linha `fr.inserir(filho, (prof + 1) + heuristica(vizinho));` — `prof + 1`
é o `g` (profundidade do novo estado) e `heuristica(vizinho)` é o `h`. Como
`PrioridadeFronteira` sempre devolve quem tem a menor "chave" (é assim que
o heap foi programado — seção 3.1), o efeito de "examinar o mais
promissor primeiro" sai de graça, só por causa de qual estrutura foi
escolhida.

**Ponto forte, medido de verdade:** nos testes feitos durante o
desenvolvimento, o A\* visitou de **9 a 35 vezes menos estados** que as
outras duas pra achar a mesma solução ótima — mesmo a heurística sendo
relativamente fraca (só varia de 0 a 2).

### 3.6.4 — Comparando as três de relance

| | Estrutura usada | Usa heurística? | Visita quantos estados? | Garante o ótimo? |
|---|---|---|---|---|
| Busca em Largura | Fila (FIFO) | Não | Muitos | Sim |
| Prof. Limitada Iterativa | Pilha (LIFO) + limite crescente | Não | Muitos (soma de todas as rodadas) | Sim |
| A\* | Fila de prioridade (heap) | Sim | Poucos | Sim (porque a heurística é admissível/consistente) |

As três **sempre concordam** no comprimento da solução (nenhuma "chuta" nem
faz nada aleatório) — a única coisa que muda de verdade é **quanto
trabalho** cada uma precisa fazer pra chegar lá.

---

# PARTE 4 — `Teclado.cpp`: ler uma tecla sem precisar de Enter

```cpp
#ifdef _WIN32
#include <conio.h>

int lerTecla()
{
    int c = _getch();
    if (c == 0 || c == 224) _getch();
    return c;
}
#else
... (versao Linux/macOS)
#endif
```

`_getch()` é uma função pronta (da biblioteca `conio.h` do Windows) que
**bloqueia** (para e espera) até você apertar uma tecla, e devolve o
caractere na hora, sem precisar de Enter e sem mostrar a tecla na tela.

Setas e teclas de função mandam **2 bytes**: primeiro um código especial
(`0` ou `224`), depois o código real. O `if` descarta esse segundo byte,
pra ele não "sobrar" e ser confundido com outra tecla depois.

A versão de Linux/macOS (fora do `#ifdef`) faz a mesma coisa de outro
jeito: coloca o terminal em "modo bruto" temporariamente, lê 1 caractere
puro, e devolve o terminal ao normal.

---

# PARTE 5 — `Janela3D.cpp`: o extra opcional (3D de verdade, com raylib)

Essa parte só existe se o programa for compilado com `make 3d` (variável
`COM_JANELA_3D`). Não é obrigatória pro trabalho — é o extra que o
enunciado permite ("interface 3D"). Resumo de como funciona, sem entrar em
todo detalhe (é a parte mais complexa do projeto):

- **Geometria:** duas tabelas, `POS` (onde fica cada um dos 8 cantos no
  espaço) e `DIR` (pra onde apontam os 3 adesivos visíveis de cada canto) —
  mesma ideia de tabela fixa que vimos com `PERM`/`TWIST`, só que agora
  pra desenhar em 3D em vez de girar o cubo.
- **`desenharCanto()`:** desenha o corpo preto de uma peça (`DrawCube`, uma
  função pronta do raylib) e os 3 adesivos coloridos colados nas faces
  certas, pegando a cor de `facelets(cubo)` — a mesma função do `Cubo.cpp`
  que a planificação usa! Não existe uma "segunda versão" do cubo aqui, é o
  mesmo estado sendo desenhado de outro jeito.
- **Animação:** quando uma face está girando, as 4 peças daquele lado são
  desenhadas dentro de um bloco `rlPushMatrix()`/`rlRotatef()`/
  `rlPopMatrix()` (funções do raylib que rotacionam tudo que for desenhado
  ali dentro) — as outras 4 peças são desenhadas normalmente, por fora
  desse bloco.
- **`abrirJanela3D()`:** um laço parecido com o do `main()` (`while
  (!WindowShouldClose())`), só que aqui quem lê teclado é o próprio raylib
  (`IsKeyPressed`), e a cada volta atualiza a câmera, processa teclas,
  avança a animação em andamento, e desenha tudo de novo — 60 vezes por
  segundo.
- Ela recebe `cubo`, `historico`, `temSolucao` e `ultimaBusca` **por
  referência** — ou seja, mexe diretamente nas mesmas variáveis do
  `main.cpp`. Quando a janela fecha, o console continua exatamente de onde
  a janela deixou.

---

# PARTE 6 — Perguntas prováveis, com respostas curtas

**"Por que fixar a peça DBL?"**
Porque o 2x2x2 não tem centro, então girar o cubo inteiro na mão não muda o
quebra-cabeça — só a representação dele. Fixando uma peça de referência, o
código nunca precisa lidar com 24 versões redundantes do mesmo estado.

**"O laço de busca realmente não muda entre as três estratégias?"**
Não — `lacoDeBusca()` (`Busca.cpp`) é chamada pelas três funções públicas
com exatamente a mesma assinatura. Só muda qual `Fronteira` concreta é
criada antes de chamar (`FilaFronteira`, `PilhaFronteira` ou
`PrioridadeFronteira`).

**"Por que a heurística do A\* garante a solução ótima?"**
Porque ela é admissível (nunca superestima o custo real) e consistente (o
valor muda no máximo 1 por movimento) — provado na seção 3.4.

**"Por que a Profundidade Iterativa visita mais estados que a Largura, se
as duas acham a mesma solução ótima?"**
Porque ela roda o laço várias vezes (uma por limite de profundidade), e
cada rodada começa do zero — o total reportado é a soma de todas as
rodadas, inclusive as que não acharam nada.

**"Como o código sabe que um estado já foi visitado?"**
Cada estado vira um número único (`indiceDoEstado()`, código de Lehmer +
orientação em base 3), usado como índice de um vetor (`menorProfundidade`)
— consulta em tempo constante, sem precisar comparar cubo por cubo.

**"Onde está o Estado, a Função Sucessora e a Função Avaliadora,
especificamente?"**
Estado: `struct Cubo` (`Cubo.hpp`). Função sucessora: `mover()`
(`Cubo.cpp`). Função avaliadora: `ehObjetivo()` (`Busca.cpp`), que só chama
`estaResolvido()` (`Cubo.cpp`).

---

# PARTE 7 — Respostas diretas, pra consulta rápida antes da arguição

## 7.1 — Onde está implementado o Estado, a Função Sucessora e a Função Avaliadora?

**Estado** — `struct Cubo` (`Cubo.hpp:13-16`):
```cpp
struct Cubo {
    std::array<uint8_t, N_CANTOS> cp;   // qual peça está em cada casa
    std::array<uint8_t, N_CANTOS> co;   // orientação de cada peça
};
```
Ver seção 2.1 pra entender `cp`/`co` por dentro.

**Função sucessora** — `mover()` (`Cubo.cpp:49`):
```cpp
Cubo mover(const Cubo &origem, int movimento)
```
Recebe um estado e um movimento (0-8), devolve o estado seguinte. Por
dentro, usa `umQuartoDeVolta()` (`Cubo.cpp:38`) e as tabelas `PERM`/`TWIST`
(`Cubo.cpp:13-22`). Ver seções 2.3 a 2.5.

**Função avaliadora** — `ehObjetivo()` (`Busca.cpp:48`):
```cpp
bool ehObjetivo(const Cubo &c) { return estaResolvido(c); }
```
Só repassa pra `estaResolvido()` (`Cubo.cpp:31`), que confere se cada uma
das 8 casas tem a peça certa e sem torção. Ver seção 2.2 e 3.4.

## 7.2 — Onde e o que são os métodos resolvidos por IA?

As três ficam no `Busca.cpp`, como funções públicas (ver seção 3.6 pra
explicação completa de cada uma):

| Função | Linha | O que é |
|---|---|---|
| `buscaEmLargura()` | `Busca.cpp:109` | Busca em Largura (BFS) — examina todos os estados de uma profundidade antes de ir pra próxima. Usa `FilaFronteira`. |
| `buscaProfundidadeIterativa()` | `Busca.cpp:135` | Profundidade Limitada Iterativa (IDDFS) — repete a busca aumentando o limite de profundidade (0, 1, 2...) até achar. Usa `PilhaFronteira`. |
| `buscaAEstrela()` | `Busca.cpp:122` | A\* — usa a heurística (seção 3.4) pra examinar primeiro o estado que parece mais promissor. Usa `PrioridadeFronteira`. |

As três são chamadas pelo `main.cpp` dentro de `resolver()` (`main.cpp:141`,
teclas `1`/`2`/`3`) e de `compararTodas()` (`main.cpp:167`, tecla `m`).

## 7.3 — Onde está o laço pedido na implementação da busca?

`lacoDeBusca()`, função **estática** (só visível dentro do próprio arquivo)
em `Busca.cpp:58`:

```cpp
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
```

É **o mesmo código**, sem nenhuma linha diferente, usado pelas três buscas
da seção 7.2 — a identidade de cada estratégia vem inteiramente de qual
`Fronteira` concreta é passada no parâmetro `fr` (fila, pilha ou fila de
prioridade — `Fronteira.hpp`). Explicação completa, passo a passo, na
seção 3.5.

## 7.4 — Como foi implementado o 3D?

Em `Janela3D.cpp`, usando a biblioteca gráfica **raylib** (só compila com
`make 3d` — é um extra opcional, não obrigatório pro trabalho). Resumo
(detalhes na Parte 5):

- Duas tabelas fixas, `POS` e `DIR` (`Janela3D.cpp:10-30`), dizem onde cada
  um dos 8 cantos fica no espaço e pra onde apontam seus 3 adesivos — mesma
  ideia de tabela "escrita na mão uma vez" que `PERM`/`TWIST` usam no
  `Cubo.cpp`.
- `desenharCanto()` (`Janela3D.cpp:53`) desenha cada peça: um cubo preto
  (`DrawCube`, função pronta do raylib) mais 3 adesivos coloridos colados
  nas faces certas — a cor de cada adesivo vem de `facelets(cubo)`, a
  **mesma função** que a planificação em texto usa (`Cubo.cpp`). Não existe
  uma segunda versão do cubo só pro 3D.
- A animação de um giro usa `rlPushMatrix()`/`rlRotatef()`/`rlPopMatrix()`
  (funções do raylib que rotacionam tudo que for desenhado dentro desse
  bloco) só nas 4 peças da face que está girando — as outras 4 são
  desenhadas normalmente, por fora.
- `abrirJanela3D()` (`Janela3D.cpp:78`) tem um laço parecido com o do
  `main()` (`while (!WindowShouldClose())`), só que quem lê teclado é o
  próprio raylib (`IsKeyPressed`), rodando 60 vezes por segundo.
- Recebe `cubo`, `historico`, `temSolucao` e `ultimaBusca` **por
  referência** direto do `main.cpp` — move no mesmo estado da sessão do
  console, não numa cópia separada.

## 7.5 — Por que digitar um número muito grande ou uma letra na semente trava o programa?

**As duas situações são, por baixo dos panos, exatamente o mesmo bug** —
não são dois problemas diferentes. A causa está em `embaralharAgora()`
(`main.cpp:122`):

```cpp
unsigned int novaSemente;
if (std::cin >> novaSemente) semente = novaSemente;
```

`std::cin >> novaSemente` tenta ler um número. Isso **falha** em dois
casos:
1. Você digita algo que não é número (uma letra, um símbolo).
2. Você digita um número **grande demais** pra caber no tipo `unsigned int`
   (que vai só até `4.294.967.295`) — por exemplo, `99999999999999`. O C++
   trata isso como estouro de capacidade, e a leitura falha do mesmo jeito
   que falharia com uma letra (testei isso na prática: os dois casos
   deixam o `std::cin` exatamente no mesmo estado de erro).

**O que acontece quando essa leitura falha, nos dois casos:** o `std::cin`
entra num **estado de erro interno** e fica "travado" nele — a partir
desse momento, **toda tentativa seguinte** de ler alguma coisa dele volta a
falhar **instantaneamente**, sem nem esperar você digitar nada, até que
alguém, em algum lugar do código, mande ele **limpar** esse erro
(`std::cin.clear()`). Só que **esse código nunca faz essa limpeza em
lugar nenhum** — então, depois da primeira vez que isso acontece, o
`std::cin` fica quebrado **pelo resto da execução do programa**: toda vez
que você apertar `e` depois disso, as perguntas aparecem na tela, mas as
leituras falham na hora, e o programa sempre reaproveita os valores
antigos de `semente`/`tamanhoEmbaralho`, sem avisar que algo deu errado.

> Detalhe à parte: digitar um número **válido, mas fora do intervalo**
> pedido pra quantidade de movimentos (por exemplo, `100`, quando o pedido
> é de 1 a 40) é uma situação **diferente e mais branda** — ali a leitura
> em si **funciona** (`100` é um número válido), só a checagem de intervalo
> (`novoTamanho <= 40`) que falha, então o `std::cin` continua saudável e
> as próximas tentativas de embaralhar funcionam normalmente. O programa só
> não avisa que descartou aquele valor — mas não trava.
