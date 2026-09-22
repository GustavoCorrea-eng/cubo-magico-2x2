# Simulador de Cubo Mágico 2x2x2 com IA de busca

Trabalho em **C++ (C++17)**: um cubo mágico 2x2x2 jogável no terminal, com
visualização colorida em planificação (texto), que também se resolve sozinho
usando três estratégias de busca:

- **Busca em Largura**
- **Busca em Profundidade Limitada Iterativa**
- **A\***

As três compartilham **um único laço de busca** (`lacoDeBusca`, função estática
em `Busca.cpp`); o que muda entre elas é somente a estrutura de dados que guarda
os estados ainda não expandidos (fila, pilha ou fila de prioridade).

## Como compilar e rodar

Windows (MSYS2 / MinGW UCRT64, `g++` no PATH — nada mais para instalar):

```bash
g++ -Wall -Wextra -O2 -std=c++17 Cubo.cpp Busca.cpp Teclado.cpp main.cpp -o cubo2x2.exe
./cubo2x2.exe
```

Ou simplesmente `make` (em outros sistemas, `make` também funciona — o
programa é portável, só a leitura de tecla única tem um caminho separado para
Windows e para Linux/macOS).

Não há nenhum executável dentro da pasta do projeto — só código-fonte. Gere o
`.exe` localmente com o comando acima antes de rodar.

### Extra: janela 3D de verdade (opcional)

Esse build acima é **auto-suficiente** (sem bibliotecas de terceiros) e já
atende tudo o que o enunciado pede. Como extra, existe também uma janela 3D
de verdade (câmera, rotação, giros animados), usando a biblioteca
[raylib](https://www.raylib.com/). Ela só entra se você pedir:

```bash
# instale o raylib uma vez (MSYS2 UCRT64):
pacman -S mingw-w64-ucrt-x86_64-raylib

# compile o alvo com 3D:
make 3d
./cubo2x2-3d.exe
```

Dentro do programa, a tecla `j` abre a janela 3D (ver `Janela3D.hpp/.cpp`).
O `.exe` gerado por `make 3d` depende de `raylib.dll` e `glfw3.dll` em tempo
de execução (ficam em `ucrt64/bin` do MSYS2) — não é um binário standalone.
Sem o raylib instalado, `make` (sem `3d`) continua funcionando normalmente,
só sem a tecla `j`.

### Teclas

Todas valem sem apertar Enter (uma tecla, uma ação):

| Tecla | Ação |
|---|---|
| `u` `r` `f` | giro horário das faces U, R, F |
| `U` `R` `F` | giro anti-horário (shift + letra) |
| `z` | desfazer o último movimento |
| `e` | embaralhar (pede semente e número de movimentos) |
| `c` | voltar ao estado resolvido |
| `v` | alternar entre a planificação (6 faces) e a vista de canto (3 faces, pseudo-3D) |
| `j` | abrir a janela 3D de verdade (só existe se compilado com `make 3d`) |
| `1` `2` `3` | resolver com Largura / Profundidade Iterativa / A\* |
| `m` | rodar as três buscas de uma vez e comparar numa tabela |
| `a` | aplicar a solução encontrada, passo a passo |
| `q` | sair |

A mesma semente com o mesmo número de movimentos gera sempre o mesmo cubo, em
qualquer máquina — assim dá para "refazer" o embaralhamento e comparar as três
estratégias no mesmo caso (requisito 6 do enunciado).

## Organização do código

| Arquivo | Conteúdo |
|---|---|
| `Cubo.hpp` / `Cubo.cpp` | **Estado** (`struct Cubo`) e **função sucessora** (`mover`); índice único do estado (`indiceDoEstado`); embaralhamento reprodutível por semente (`embaralhar`); conversão do estado para as 24 "figurinhas"; impressão colorida em planificação (`imprimirCubo`) e em vista de canto/pseudo-3D (`imprimirCuboIso`) |
| `Fronteira.hpp` | As três estruturas de dados atrás de uma interface comum (classe abstrata `Fronteira`, com `FilaFronteira`, `PilhaFronteira` e `PrioridadeFronteira`) |
| `Busca.hpp` / `Busca.cpp` | **Função avaliadora** (`ehObjetivo`), **heurística** do A\* (`heuristica`), e **o laço de busca único** (`lacoDeBusca`, estático em `Busca.cpp`), chamado pelas três funções públicas `buscaEmLargura`, `buscaProfundidadeIterativa`, `buscaAEstrela` |
| `Teclado.hpp` / `Teclado.cpp` | Leitura de uma única tecla, sem precisar de Enter |
| `Janela3D.hpp` / `Janela3D.cpp` | **Extra** (só em `make 3d`): janela 3D de verdade com raylib — câmera, giros animados, mesma geometria/cores do resto do projeto |
| `main.cpp` | Laço principal: desenha a tela, lê uma tecla, aplica a ação |
| `Makefile` | `make` compila o básico; `make 3d` compila com a janela 3D; `make clean` apaga os executáveis |

## 1. Estado

Um 2x2x2 tem 8 peças de canto e nenhum centro, então a orientação do cubo
inteiro é livre. **Fixamos a peça DBL (índice 7)** e só giramos as faces
**U, R e F**, que nunca a tocam. Isso não perde generalidade (todo estado do
cubo é equivalente, por rotação do cubo inteiro, a um com a DBL no lugar) e
divide o espaço de busca por 24.

```cpp
struct Cubo {
    std::array<uint8_t, 8> cp;   // cp[i] = qual peça ocupa a posição i (permutação)
    std::array<uint8_t, 8> co;   // co[i] = orientação da peça: 0, 1 ou 2
};
```

Estado resolvido: `cp[i] == i` e `co[i] == 0` para todo `i`.

**Tamanho do espaço:** 7 peças móveis podem ser permutadas de 7! = 5040 formas
e 6 delas têm orientação livre (a sétima é determinada pelas outras, pois a
soma das torções é múltiplo de 3): 3⁶ = 729. Total: 7! × 3⁶ = **3.674.160
estados**.

**Índice do estado** (`indiceDoEstado`): código de Lehmer da permutação
(0…5039) combinado com a orientação em base 3 (0…728): `perm * 729 + orient`.
É uma bijeção sobre 0…3.674.159, o que permite marcar visitados em O(1) com um
`std::vector<uint8_t>`, sem tabela hash.

## 2. Função sucessora

São **9 movimentos**: `U U2 U'  R R2 R'  F F2 F'` (código = face·3 + variação).
`mover(origem, mov)` aplica 1, 2 ou 3 quartos de volta usando as tabelas
`PERM` (para onde vai cada peça) e `TWIST` (quanto a orientação muda).

**Poda que não perde a otimalidade** (dentro do laço de busca):

- não girar duas vezes seguidas a mesma face (`mesmaFace`): `R R2` é o mesmo
  que `R'`, e já é gerado por outro movimento;
- só reabrir um estado já visto se o novo caminho for **estritamente mais
  curto** (`menorProfundidade[idx]`). Cada estado tem um único nó na busca
  (`noDoEstado[idx]`), reaproveitado quando se acha um caminho melhor.

## 3. Função avaliadora e heurística

`ehObjetivo(c)` devolve verdadeiro quando o cubo está resolvido.

**Heurística do A\*:** `h(s) = ⌈ fora(s) / 4 ⌉`, onde `fora(s)` é o número de
cantos (entre os 7 móveis) que estão na posição errada **ou** com orientação
errada.

- **Admissível:** um movimento de face mexe em exatamente 4 posições de canto
  (U: 0‑3; R: 0,3,4,6; F: 0,1,4,5 — nenhuma inclui a DBL). Só as peças dessas
  4 posições podem mudar de "fora" para "certa", então `fora` diminui no
  máximo 4 por movimento. Faltam, portanto, pelo menos ⌈fora/4⌉ movimentos:
  `h` nunca superestima o custo real.
- **Consistente:** como `fora(s) ≤ fora(s') + 4` para todo sucessor `s'` e
  cada movimento custa 1, vale `h(s) ≤ 1 + h(s')`. Além disso `h(objetivo) =
  0`. Consistência garante que o A\* devolve a solução **ótima**.

## 4. O laço de busca único

O trecho abaixo é o coração do trabalho (`Busca.cpp`, função estática
`lacoDeBusca`). Os comentários seguem os passos numerados do enunciado:

```cpp
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    // 1. Adicionar estado na estrutura
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    ...
    fr.inserir(raiz, heuristica(inicial));

    // 2. Enquanto a estrutura nao estiver vazia
    while (!fr.vazia()) {
        // 2.1 Remover proximo estado da estrutura
        int atual = fr.remover();
        ...
        // 2.2 Avaliar estado
        if (ehObjetivo(estado)) {
            // 2.2.1 SE estado final -> mostrar solucao e encerrar
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }
        // 2.3 Adicionar estados seguintes na estrutura
        for (int m = 0; m < N_MOVIMENTOS; m++) {
            ...
            fr.inserir(filho, (prof + 1) + heuristica(vizinho));
        }
    }
    // 3. Retornar "Sem solucao"
    r.encontrou = false;
}
```

`Fronteira` é uma classe abstrata (`inserir` / `remover` / `vazia`); o laço
recebe uma referência a ela e **nunca sabe qual estrutura está por trás**.
Trocar de estratégia é só passar outra implementação — sem mudar uma linha do
laço, como exige o enunciado:

| Estratégia | Estrutura (`Fronteira.hpp`) | Como é chamado o laço |
|---|---|---|
| Busca em Largura | `FilaFronteira` (FIFO) | `lacoDeBusca(fila, mem, s, -1, r)` |
| Profundidade Limitada Iterativa | `PilhaFronteira` (LIFO) | `lacoDeBusca(pilha, mem, s, limite, r)` para `limite = 0, 1, 2, …` até achar |
| A\* | `PrioridadeFronteira` (heap binário) | `lacoDeBusca(heap, mem, s, -1, r)`, chave `f = g + h` |

Detalhes de cada uma:

- **Largura:** a fila devolve os estados por ordem de profundidade, então o
  primeiro objetivo encontrado é o de menor número de movimentos.
- **Profundidade iterativa:** o parâmetro `limite` faz o laço não expandir
  estados com `profundidade >= limite`. `buscaProfundidadeIterativa` chama o
  mesmo laço com limite 0, 1, 2… e para na primeira rodada que acha solução —
  que, por isso, é ótima.
- **A\*:** a chave de inserção é `g + h`. Em empate de `f`, sai primeiro o nó
  inserido por último (o mais profundo), o que reduz a expansão.

## 5. Resultados medidos

Embaralhamentos de 9 movimentos, `g++ -O2`, seis sementes diferentes. Os
**estados visitados** são determinísticos; os **tempos** variam com a
máquina. As três buscas sempre devolveram soluções de **mesmo comprimento**
(o ótimo), confirmado também aplicando a solução ao cubo inicial:

| Semente | Ótimo | Largura (visitados) | Prof. Iterativa (visitados) | A\* (visitados) |
|---|---|---|---|---|
| 1 | 8 | 889.866    | 803.157   | **57.203**  |
| 2 | 9 | 1.192.728  | 3.272.665 | **90.143**  |
| 3 | 9 | 2.843.466  | 3.109.243 | **102.067** |
| 4 | 9 | 2.978.106  | 2.673.648 | **66.095**  |
| 5 | 9 | 2.033.219  | 5.304.424 | **286.227** |
| 6 | 7 | 106.480    | 341.472   | **9.109**   |

O A\* visita de **9× a 35× menos estados** que as outras duas, mesmo com uma
heurística simples (`h` vai só de 0 a 2). A Profundidade Iterativa costuma
visitar mais que a Largura porque cada rodada refaz o trabalho das rodadas
anteriores (o total é a soma dos visitados em todos os limites).

## Vista de canto (pseudo-3D)

Além da planificação (as 6 faces esticadas num plano), a tecla `v` alterna
para uma segunda visualização: só as 3 faces jogáveis (U, F, R) desenhadas
juntas, com a face U levemente deslocada para a direita a cada linha, para
sugerir profundidade:

```
         U  U      (U em cima)
       U  U
 F  F    R  R
 F  F    R  R    (F na frente, R na direita)
```

Não é uma câmera 3D de verdade — não há matriz de rotação nem projeção em
perspectiva, só indentação progressiva (`imprimirCuboIso` em `Cubo.cpp`).
É uma forma barata de olhar de uma vez só para as 3 faces que se manipula,
sem precisar "traduzir" a planificação toda vez.

## Janela 3D de verdade (extra, `make 3d`)

Diferente da vista de canto acima, esta é uma projeção 3D real, com câmera
que gira (setas do teclado) e giros animados — implementada com
[raylib](https://www.raylib.com/) em `Janela3D.cpp`, aberta com a tecla `j`.

A geometria reaproveita as mesmas tabelas de posição/direção dos cantos e a
mesma tabela `CANTO_FACELET` (exposta em `Cubo.hpp`) que o resto do projeto
já usa para desenhar o cubo em texto — ou seja, não existe uma segunda
"versão" do cubo por trás da janela, é literalmente o mesmo `Cubo` sendo
desenhado de outro jeito.

Cada giro de face é animado girando, ao redor do eixo correspondente
(U → eixo Y, R → eixo X, F → eixo Z), só as 4 peças daquele lado — usando a
pilha de matrizes do raylib (`rlPushMatrix`/`rlRotatef`/`rlPopMatrix`) em vez
de recalcular vértice por vértice à mão. A correção da direção de cada
animação (todos os 9 movimentos, sentido horário/anti-horário/180°) foi
conferida comparando, pixel a pixel, o frame final da animação contra uma
renderização estática do cubo já com o movimento aplicado por `mover()` —
diferença de no máximo 1 pixel (ruído de ponto flutuante) em todos os casos.

## Requisitos do enunciado — conferência

1. **Interface** — texto colorido no terminal, mostra o cubo planificado (ou,
   alternativamente, em vista de canto) e os comandos disponíveis; qualquer
   tecla move ou aciona uma IA.
2. **Laço genérico** — `lacoDeBusca` não muda entre as três estratégias.
3. **Jogar ou IA** — a mesma tela deixa jogar (`u r f`/`U R F`) ou chamar
   qualquer uma das três IAs (`1`/`2`/`3`, ou `m` para rodar as três e comparar).
4. **Estados visitados** — mostrado após cada busca (`ultimaBusca.visitados`).
5. **Passos da solução** — impressos como a sequência de teclas para
   reproduzi-los na mão, e também aplicáveis um a um com a tecla `a`.
6. **Reprodutibilidade** — semente + número de movimentos regeneram sempre o
   mesmo cubo (`e`).

## Limitações conhecidas

- A vista de canto (`v`) é uma aproximação por indentação, não uma projeção
  3D real; a planificação continua sendo a única visualização com as 6 faces.
- Enquanto a Busca em Largura roda em cubos muito embaralhados (17+
  movimentos), o programa fica alguns segundos sem responder — é esperado,
  porque a busca é síncrona (sem threads).
- A janela 3D (tecla `j`) é só para jogar/visualizar: gira faces, desfaz,
  reseta e aplica (com animação) a solução que **já foi calculada no
  console** antes de abrir a janela. Rodar uma busca nova (`1`/`2`/`3`/`m`)
  só é possível no console — feche a janela (`ESC`) para acessá-las.
- A janela 3D (`make 3d`) não é standalone: precisa do raylib instalado para
  compilar, e de `raylib.dll`/`glfw3.dll` em tempo de execução. O build
  básico (`make`, sem `3d`) não tem essa dependência.
