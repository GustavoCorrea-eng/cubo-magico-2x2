# Especificação Técnica — Cubo Mágico 2x2x2

Documento de apoio para a arguição: explica cada arquivo, cada função
relevante, as decisões de projeto e por quê, e traz um roteiro de perguntas
prováveis com respostas prontas. Para instruções de uso, veja `MANUAL.md`; a
visão geral e as provas resumidas também estão no `README.md`.

## 1. Arquitetura

```
Cubo.hpp/.cpp ─── estado + função sucessora + índice + impressão colorida
      │
      ▼
Fronteira.hpp ─── fila / pilha / heap, atrás de uma interface comum
      │
      ▼
Busca.hpp/.cpp ── função avaliadora + heurística + O LAÇO ÚNICO + 3 estratégias
      │
      ▼
main.cpp ──────── interface de texto (usa Cubo.hpp, Busca.hpp, Teclado.hpp)
      ▲
      │
Teclado.hpp/.cpp ─ leitura de uma tecla sem Enter
```

Nenhum arquivo depende de `main.cpp`. `Busca.cpp` só conhece `Cubo` e
`Fronteira` — nunca soube que existe uma interface de texto por cima.

## 2. Estado (`Cubo.hpp`)

```cpp
struct Cubo {
    std::array<uint8_t, 8> cp;   // cp[i] = peça que ocupa a posição i
    std::array<uint8_t, 8> co;   // co[i] = orientação da peça: 0, 1 ou 2
};
```

**Por que 8 posições e só 7 "móveis"?** Um 2x2x2 tem 8 peças de canto e
nenhuma peça de centro. Sem centros, girar o cubo inteiro no espaço não muda
o quebra-cabeça — só muda de qual lado você está olhando. Isso significa que
a orientação do cubo como bloco rígido é irrelevante para resolver, então
**fixamos a posição 7 (canto DBL)** e só permitimos girar as três faces que
nunca tocam essa posição: **U, R, F**. Toda configuração alcançável do cubo
tem uma representação equivalente com a peça DBL fixa — não perdemos nenhum
caso, só evitamos representar 24 vezes (as 24 rotações do cubo inteiro) o
mesmo quebra-cabeça.

**Tamanho do espaço de estados:** as 7 peças móveis podem ser permutadas de
`7! = 5040` formas. Cada uma tem orientação 0, 1 ou 2, mas a soma de todas as
orientações é sempre múltiplo de 3 (invariante do cubo) — logo 6 delas são
livres e a sétima é consequência: `3^6 = 729` combinações de orientação.
Total: `7! × 3^6 = 3.674.160` estados — a constante `N_ESTADOS` em `Cubo.hpp`.

**Índice único do estado** — `uint32_t indiceDoEstado(const Cubo &c)`
(`Cubo.cpp:76`): combina duas contas num único inteiro de 0 a 3.674.159:

- **Código de Lehmer** da permutação `cp[0..6]`: para cada posição `i`, conta
  quantas posições depois dela (`j > i`) têm peça menor (`cp[j] < cp[i]`), e
  acumula em base fatorial decrescente (`perm = perm*(7-i) + menores`). Isso
  dá um número de 0 a 5039 que identifica a permutação de forma única.
- **Orientação em base 3**: `co[0..5]` lidos como dígitos de um número em
  base 3, de 0 a 728.
- `indice = perm * 729 + orient` — uma bijeção. Isso permite marcar "estado
  já visto" com **um vetor simples** (`std::vector<uint8_t>` ou
  `std::vector<int>`), sem precisar de tabela hash, e sem colisões.

*Exemplo pequeno para explicar na arguição*: se pedirem para mostrar que a
função realmente é uma bijeção, o argumento é que o código de Lehmer é uma
bijeção clássica entre permutações de N elementos e os inteiros
`0..N!-1` (é a forma como se converte uma permutação para índice de fatorial
misto), e a leitura de `co` em base 3 é trivialmente uma bijeção entre
sequências de 6 dígitos ternários e `0..728`. O produto cartesiano de duas
bijeções, combinado como `a*729+b`, é bijeção sobre o produto dos intervalos.

## 3. Função sucessora (`Cubo.cpp`)

```cpp
Cubo mover(const Cubo &origem, int movimento);   // Cubo.cpp:59
```

Cada um dos 9 movimentos tem código `face*3 + variacao`, com `face` em
`{0=U, 1=R, 2=F}` e `variacao` em `{0=90° horário, 1=180°, 2=90°
anti-horário}`.

A função de base é `umQuartoDeVolta(o, face)` (`Cubo.cpp:47`), que usa duas
tabelas fixas por face:

- `PERM[face][i]`: de qual posição vem a peça que passa a ocupar `i`, num
  giro de 90°.
- `TWIST[face][i]`: quantos "terços de giro" (120°) a peça ganha ao chegar
  em `i`.

`mover()` aplica `umQuartoDeVolta` 1, 2 ou 3 vezes seguidas, conforme a
variação do movimento. As tabelas `PERM`/`TWIST` foram obtidas mapeando
manualmente, para cada face, quais das 8 posições de canto trocam de lugar
num giro de 90° e quanto cada peça gira ao entrar na nova posição — é
geometria fixa do cubo, não depende do estado.

## 4. Estrutura de dados da busca (`Fronteira.hpp`)

```cpp
class Fronteira {
public:
    virtual void inserir(int idNo, int chave) = 0;
    virtual int  remover() = 0;
    virtual bool vazia() const = 0;
    virtual const char *nome() const = 0;
};
```

Três implementações, cada uma só troca a política de qual nó sai primeiro:

| Classe | Estrutura interna | Política de saída | Usada por |
|---|---|---|---|
| `FilaFronteira` | `std::deque<int>` | primeiro que entrou (FIFO) | Busca em Largura |
| `PilhaFronteira` | `std::vector<int>` | último que entrou (LIFO) | Profundidade Limitada |
| `PrioridadeFronteira` | `std::vector<Item>` + heap binário | menor `chave` primeiro | A\* |

`PrioridadeFronteira` usa `std::push_heap`/`std::pop_heap` com um
comparador que inverte o sentido padrão do `std::priority_queue` (que por
padrão é *max-heap*), para funcionar como *min-heap* — quem tem menor `f =
g + h` sai primeiro. Em empate de `f`, desempata pela ordem de inserção
(quem entrou por último sai primeiro), o que na prática favorece nós mais
profundos e reduz a expansão.

**Por que classe abstrata (polimorfismo) em vez de `struct` com ponteiros de
função?** É a forma idiomática de C++ de fazer exatamente a mesma coisa: o
laço de busca recebe uma `Fronteira&` e chama métodos virtuais sem saber a
classe concreta por trás — o *dispatch* dinâmico do C++ substitui os
ponteiros de função manuais que a mesma ideia usaria em C.

## 5. Função avaliadora e heurística (`Busca.cpp`)

```cpp
bool ehObjetivo(const Cubo &c) { return estaResolvido(c); }        // Busca.cpp:62
int  heuristica(const Cubo &c);                                     // Busca.cpp:78
```

`heuristica(c) = ⌈ fora(c) / 4 ⌉`, onde `fora(c)` é a quantidade de cantos
(entre os 7 móveis, índices 0 a 6) com `cp[i] != i` **ou** `co[i] != 0`.

**Prova de admissibilidade** (nunca superestima o custo real até a
solução): cada um dos 9 movimentos gira uma única face, e cada face mexe em
exatamente 4 das 7 posições de canto móveis:

- `U` mexe nas posições 0, 1, 2, 3
- `R` mexe nas posições 0, 3, 4, 6
- `F` mexe nas posições 0, 1, 4, 5

(nenhuma inclui a posição 7, fixa por construção). Logo, um único movimento
só pode "acertar" (tirar de `fora`) no máximo 4 peças. Se há `fora` peças
erradas, são necessários pelo menos `⌈fora/4⌉` movimentos para zerá-las —
exatamente o valor que a heurística devolve. Como o custo real nunca é menor
que isso, `h` nunca superestima: é **admissível**.

**Prova de consistência** (`h(s) ≤ custo(s,s') + h(s')` para todo sucessor
`s'`): um movimento muda `fora` em no máximo 4 (pode diminuir até 4 ou
aumentar até 4, nunca mais, pelo mesmo argumento acima). Logo `⌈fora/4⌉`
muda no máximo 1 por movimento, e como cada movimento custa 1:
`h(s) ≤ 1 + h(s')`. Consistência é uma condição mais forte que admissibilidade
e garante, pela teoria de A\*, que **a primeira vez que o algoritmo retira um
nó da fronteira e ele é o objetivo, o caminho até ele já é ótimo** — não é
preciso reabrir nós expandidos.

`h` é uma heurística fraca (varia só de 0 a 2, já que `fora` vai no máximo a
7 e `⌈7/4⌉ = 2`), mas mesmo assim reduz drasticamente os estados visitados
(ver seção 8) — é um bom ponto para comentar na arguição que uma heurística
mais forte (ex.: um *pattern database*) reduziria ainda mais, mas não foi
necessária para o desempenho exigido.

## 6. O laço de busca único (`Busca.cpp:101`)

```cpp
static void lacoDeBusca(Fronteira &fr, MemoriaDaBusca &mem, const Cubo &inicial,
                        int limite, Resultado &r)
{
    // 1. Adicionar estado na estrutura
    int raiz = mem.novoNo(inicial, -1, 0, 0);
    mem.menorProfundidade[indiceDoEstado(inicial)] = 0;
    mem.noDoEstado[indiceDoEstado(inicial)] = raiz;
    fr.inserir(raiz, heuristica(inicial));
    r.gerados++;

    // 2. Enquanto a estrutura nao estiver vazia
    while (!fr.vazia()) {
        // 2.1 Remover proximo estado da estrutura
        int atual = fr.remover();
        Cubo estado = mem.pool[atual].cubo;
        int prof    = mem.pool[atual].profundidade;
        r.visitados++;

        // 2.2 Avaliar estado
        if (ehObjetivo(estado)) {
            // 2.2.1 SE estado final -> mostrar solucao e encerrar
            mem.reconstruirCaminho(atual, r);
            r.encontrou = true;
            return;
        }

        // 2.3 Adicionar estados seguintes na estrutura
        if (limite >= 0 && prof >= limite) continue;
        for (int m = 0; m < N_MOVIMENTOS; m++) {
            if (mem.pool[atual].pai >= 0 && mesmaFace(m, mem.pool[atual].movimento))
                continue;                                    // poda: mesma face 2x
            Cubo vizinho = mover(estado, m);
            uint32_t idx = indiceDoEstado(vizinho);
            if (mem.menorProfundidade[idx] <= prof + 1) continue;   // ja visto por caminho <=
            mem.menorProfundidade[idx] = prof + 1;
            int filho = mem.noDoEstado[idx];
            if (filho >= 0) { /* reaproveita o no, so atualiza pai/movimento/profundidade */ }
            else            { filho = mem.novoNo(vizinho, atual, m, prof + 1);
                               mem.noDoEstado[idx] = filho; }
            fr.inserir(filho, (prof + 1) + heuristica(vizinho));
            r.gerados++;
        }
    }
    // 3. Retornar "Sem solucao"
    r.encontrou = false;
}
```

Este é **o** requisito central do enunciado: o laço acima é usado, sem
nenhuma alteração, pelas três estratégias — só muda qual `Fronteira`
concreta é passada em `fr` e (no caso da Profundidade Iterativa) o valor de
`limite`. A correspondência com os passos numerados do enunciado está nos
comentários (1 / 2 / 2.1 / 2.2 / 2.2.1 / 2.3 / 3), palavra por palavra.

**Duas podas**, ambas sem custo de otimalidade (não descartam nenhuma
solução ótima):

1. **Não repetir a mesma face duas vezes seguidas** (`mesmaFace`): girar a
   mesma face de novo (ex.: `R` seguido de `R2`) sempre equivale a um único
   movimento diferente (nesse exemplo, `R'`) que já é gerado por outro ramo
   — evita ramos redundantes sem perder nenhum estado alcançável.
2. **Só reabrir um estado por um caminho estritamente mais curto**
   (`menorProfundidade[idx]`): cada estado tem no máximo **um** nó vivo na
   busca (`noDoEstado[idx]`), reaproveitado quando se acha um caminho
   melhor. Isso evita duplicar nós indefinidamente num grafo cheio de ciclos
   como o do cubo (qualquer sequência de movimentos e sua inversa formam um
   ciclo).

`MemoriaDaBusca` (`Busca.cpp`, logo acima do laço) é uma pequena classe que
junta o *pool* de nós (`std::vector<No>`) e os dois vetores de controle
(`menorProfundidade`, `noDoEstado`), alocados **uma vez por chamada** e
liberados automaticamente pelo destrutor do `std::vector` ao sair de escopo
(RAII) — não há `free`/`liberar` manual, ao contrário de uma implementação
em C pura.

## 7. As três estratégias públicas (`Busca.cpp:167,180,196`)

| Função | Fronteira usada | Parâmetro extra | Observação |
|---|---|---|---|
| `buscaEmLargura` | `FilaFronteira` | — | primeira solução achada já é ótima (FIFO expande por camada) |
| `buscaAEstrela` | `PrioridadeFronteira` | — | chave `f = profundidade + heuristica(vizinho)` |
| `buscaProfundidadeIterativa` | `PilhaFronteira` | `limiteMaximo` | chama `lacoDeBusca` em rodadas com `limite = 0, 1, 2, ...` |

`buscaProfundidadeIterativa` (aprofundamento iterativo) é a única que chama
`lacoDeBusca` mais de uma vez: a cada rodada, `mem.reiniciar()` limpa os
vetores de controle (sem realocar memória) e o laço roda de novo com um
limite maior, **até a primeira rodada que encontra uma solução** — que,
justamente por ser a primeira rodada bem-sucedida, tem o menor número de
movimentos possível. `total.visitados` e `total.gerados` somam todas as
rodadas, então essa estratégia sempre reporta mais estados visitados que as
outras duas para o mesmo cubo (ela refaz o trabalho de cada rodada anterior).
`main.cpp` chama essa função com `limiteMaximo = 11`, porque 11 é o diâmetro
conhecido do espaço de estados do 2x2x2 (nenhum estado precisa de mais de 11
movimentos para ser resolvido) — se perguntarem por que 11, essa é a
resposta, embora o código não dependa desse valor para estar correto (só
para não rodar rodadas inúteis além do necessário).

## 8. Complexidade e desempenho

- **Memória por busca**: cada nó (`struct No`) guarda um `Cubo` (16 bytes) +
  pai + movimento + profundidade — poucos bytes. No pior caso (Busca em
  Largura percorrendo quase todo o espaço), o *pool* pode chegar perto de
  3.674.160 nós. Os dois vetores de controle somam
  `N_ESTADOS × (1 + 4) bytes ≈ 18,4 MB` — pouco para uma máquina atual.
- **Tempo**: determinístico em número de estados visitados (não depende da
  máquina); o tempo em segundos depende do hardware.

**Resultados medidos** (embaralhamentos de 9 movimentos, seis sementes,
`-O2`; ver `README.md` para a tabela completa e a metodologia): o A\*
visitou de **9× a 35× menos estados** que a Busca em Largura e que a
Profundidade Iterativa, para a mesma solução ótima.

## 9. Testes realizados durante o desenvolvimento

Antes da entrega, foram validados (fora do executável final, num programa de
teste à parte, para não deixar binários extras no projeto):

- Cubo resolvido → as três buscas devolvem 0 passos, e `heuristica == 0`.
- Em 50 sementes de embaralhamento, para cada estado e cada um dos 9
  movimentos: `heuristica(s) <= 1 + heuristica(sucessor)` (consistência) e
  `indiceDoEstado(sucessor) < N_ESTADOS` (índice sempre no intervalo válido).
- Em 6 sementes, as três buscas acham soluções de **mesmo comprimento**
  (ótimo), e aplicar a sequência de movimentos devolvida por cada uma delas
  ao cubo inicial realmente resolve o cubo.

## 10. `main.cpp` — interface

Não há laço de eventos nem janela: cada iteração do `main()` desenha a tela
(`desenharTela`), bloqueia esperando **uma** tecla (`lerTecla`, de
`Teclado.hpp`) e trata essa tecla (`tratarTecla`). Como a leitura é
bloqueante e não há nada rodando "ao mesmo tempo" (sem thread, sem
animação), o programa é simples de acompanhar: nunca há duas coisas
acontecendo ao mesmo tempo. O preço disso é que, enquanto uma busca roda
(`resolver`), a tela fica parada até ela terminar — aceitável porque a
busca em si é síncrona e o enunciado não pede concorrência.

`jogar(mov)` aplica o movimento e empilha em `historico` (usado por
`desfazer`, que aplica `movimentoInverso` e desempilha). `embaralharAgora`
troca temporariamente para leitura com `std::cin >>` (para digitar números
com Enter, diferente do resto da interface que lê uma tecla só) — isso é
seguro porque `lerTecla` (via `_getch`/modo bruto) não interfere no *buffer*
do `std::cin`.

`teclasDoMovimento(mov)` (`main.cpp:35`) traduz um código de movimento para
as teclas que o jogador apertaria na mão (minúscula = horário, maiúscula =
anti-horário, letra repetida = 180°) — usada tanto na mensagem de `jogar()`
quanto na lista de passos da solução (`nomesDosPassos`, `main.cpp:47`), para
que a solução mostrada seja diretamente "digitável", sem tradução de notação.

`compararTodas()` (`main.cpp:159`, tecla `m`) roda as três estratégias no
mesmo cubo em sequência e imprime uma tabela (estados visitados, gerados,
tamanho da solução e tempo, lado a lado) — útil para mostrar na arguição, ao
vivo, que o A\* visita muito menos estados que as outras duas para a mesma
solução ótima.

`imprimirCuboIso()` (`Cubo.cpp`, ver seção 2) é chamada por `desenharTela()`
em vez de `imprimirCubo()` quando `modoIsometrico` está ligado (tecla `v`) —
mostra só as 3 faces jogáveis (U, F, R) com um leve deslocamento por linha
para sugerir profundidade. Não é uma projeção 3D real (sem matriz de
rotação nem perspectiva), só uma segunda forma de olhar o cubo mais rápida
que a planificação completa.

## 11. Checklist do enunciado → onde está no código

| Requisito | Onde |
|---|---|
| Estado | `struct Cubo` (`Cubo.hpp:39`) |
| Função sucessora | `mover()` (`Cubo.cpp:59`) |
| Função avaliadora | `ehObjetivo()` (`Busca.cpp:62`) |
| Interface de visualização/manipulação | `imprimirCubo()` e `imprimirCuboIso()` (`Cubo.cpp`) + `main.cpp` inteiro |
| Busca em Largura | `buscaEmLargura()` (`Busca.cpp:167`) |
| Profundidade Limitada Iterativa | `buscaProfundidadeIterativa()` (`Busca.cpp:196`) |
| A\* com heurística | `buscaAEstrela()` (`Busca.cpp:180`) + `heuristica()` (`Busca.cpp:78`) |
| Laço único, independente da estrutura | `lacoDeBusca()` (`Busca.cpp:101`) |
| Jogar ou escolher IA | `tratarTecla()` (`main.cpp:237`) |
| Contagem de estados visitados | `Resultado::visitados`, exibido em `desenharTela()` e em `compararTodas()` (`main.cpp:159`) |
| Passos da solução de forma intuitiva | `teclasDoMovimento()`/`nomesDosPassos()` (`main.cpp:35,47`) + `aplicarSolucao()` (`main.cpp:217`) |
| Cubo inicial refeito por semente | `embaralhar()` (`Cubo.cpp:103`) + `embaralharAgora()` (`main.cpp:112`) |

## 12. Perguntas prováveis na arguição

**"Por que vocês não usaram uma representação com facelets (as 24
figurinhas) direto, em vez de permutação e orientação?"**
Usamos permutação+orientação porque dá um índice único e compacto (0 a
3.674.159) por conta própria, sem precisar de hash — isso é o que faz o
`std::vector` de visitados funcionar em O(1). Com facelets teríamos que usar
uma tabela hash (mais lenta e mais complexa) para o mesmo efeito. A
conversão para facelets (`facelets()`, `Cubo.cpp`) só existe para a
impressão colorida.

**"O laço realmente não muda entre as três buscas? Provem."**
Sim: `lacoDeBusca` (`Busca.cpp:101`) é chamado nas três funções públicas
(`Busca.cpp:167,180,196`) exatamente com a mesma assinatura, e a única coisa
que muda entre as chamadas é qual subclasse de `Fronteira` é instanciada
(`FilaFronteira`, `PilhaFronteira` ou `PrioridadeFronteira`) — o corpo da
função `lacoDeBusca` não tem nenhum `if` que diferencie estratégias.

**"Por que a Profundidade Iterativa visita mais estados que a Largura, se
as duas são exaustivas?"**
Porque a Profundidade Iterativa roda o laço várias vezes (uma por limite de
profundidade, de 0 até achar a solução), e cada rodada começa do zero —
refaz o trabalho de todas as rodadas anteriores. O total reportado é a soma
de todas as rodadas.

**"O A\* é sempre ótimo? Por quê?"**
Sim, porque a heurística é consistente (prova na seção 5) — consistência
garante que, quando um nó é retirado da fronteira pela primeira vez, o
caminho até ele já é o mais curto possível, então não é preciso reabrir nós
já expandidos.

**"O que aconteceria se a heurística superestimasse o custo?"**
O A\* deixaria de garantir a solução ótima — poderia expandir um nó "barato
demais" (com f artificialmente alto) depois de já ter achado uma solução
subótima, e parar cedo demais achando que aquele era o melhor caminho.

**"Por que só U, R e F, e não as 6 faces?"**
Explicado na seção 2: sem peças de centro, o cubo inteiro pode ser girado
livremente no espaço sem mudar o quebra-cabeça, então fixamos uma peça
(DBL) e as 3 faces que a tocam (D, L, B) tornam-se redundantes — qualquer
sequência com elas tem uma sequência equivalente usando só U, R, F.

**"Como vocês testaram que a implementação está correta?"**
Seção 9: um programa de teste separado (não entregue, para não deixar
binário extra) validou consistência da heurística, o intervalo do índice de
estado, e que as três buscas concordam no comprimento ótimo e que a solução
de cada uma realmente resolve o cubo, em várias sementes.
