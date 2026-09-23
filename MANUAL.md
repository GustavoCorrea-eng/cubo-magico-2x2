# Manual do Usuário — Cubo Mágico 2x2x2

Este manual explica como usar o programa: como jogar manualmente e como pedir
para a inteligência artificial resolver o cubo. Não é preciso saber programar
para acompanhar este documento.

## 1. Abrindo o programa

Dentro da pasta do projeto, gere o executável uma vez:

```bash
g++ -Wall -Wextra -O2 -std=c++17 Cubo.cpp Busca.cpp Teclado.cpp main.cpp -o cubo2x2.exe
```

E rode:

```bash
.\cubo2x2.exe
```

O programa roda direto no terminal — não abre nenhuma janela separada.

> Use o PowerShell, o terminal do VS Code ou o Windows Terminal. O `cmd.exe`
> antigo pode não mostrar as cores corretamente.

Se você também quiser a janela 3D de verdade (tecla `j`, ver seção 2.2),
instale o [raylib](https://www.raylib.com/) uma vez e compile com `make 3d`
em vez do comando acima:

```bash
pacman -S mingw-w64-ucrt-x86_64-raylib   # so na primeira vez
make 3d
.\cubo2x2-3d.exe
```

Nunca instalou o MSYS2/raylib antes, ou deu algum erro nesse caminho? O
`INSTALACAO_3D.md` tem o passo a passo completo, do zero, com solução dos
problemas mais comuns.

Isso é opcional — o programa funciona inteiro sem isso, só sem a tecla `j`.

## 2. Entendendo a tela

Toda vez que você faz algo, a tela é redesenhada e mostra, de cima para
baixo:

1. **O cubo**, desenhado "planificado" (como se você abrisse a caixa do cubo
   e esticasse todas as faces num plano). Cada quadradinho colorido é uma
   peça do cubo, com uma letra dentro dizendo a cor:

   | Letra | Cor | Face |
   |---|---|---|
   | `U` | branco | de cima (*Up*) |
   | `D` | amarelo | de baixo (*Down*) |
   | `F` | verde | da frente (*Front*) |
   | `R` | vermelho | da direita (*Right*) |
   | `L` | laranja | da esquerda (*Left*) |
   | `B` | azul | de trás (*Back*) |

   A disposição na tela é:

   ```
                U  U
                U  U
    L  L    F  F    R  R    B  B
    L  L    F  F    R  R    B  B
                D  D
                D  D
   ```

   Ou seja: a face de cima aparece em cima, a de baixo embaixo, e as quatro
   faces laterais (esquerda, frente, direita, trás) aparecem lado a lado no
   meio, na ordem em que você as veria girando o cubo na sua frente.

2. **Informações do estado atual**: quantos movimentos já foram feitos, qual
   a semente usada para embaralhar e se o cubo está resolvido.

3. **O resultado da última busca de IA**, se você já pediu uma (quantos
   movimentos a solução tem, quantos estados a IA visitou, quanto tempo
   levou, e a lista de movimentos da solução).

4. **O menu de teclas**, sempre visível, para lembrar os comandos.

5. **Uma mensagem** na última linha, dizendo o que aconteceu na sua última
   ação (ex.: "Movimento R aplicado.").

### 2.1. A outra vista: "de canto" (tecla `v`)

Apertando `v`, a tela troca a planificação (as 6 faces esticadas num plano)
por uma **vista de canto**: só as 3 faces que você giraria manualmente — U
(topo), F (frente) e R (direita) — desenhadas juntas, com a face U um pouco
deslocada para a direita a cada linha, pra dar uma ligeira sensação de
profundidade:

```
         U  U      (U em cima)
       U  U
 F  F    R  R
 F  F    R  R    (F na frente, R na direita)
```

Não é uma câmera 3D de verdade (não gira, não tem perspectiva real) — é só
um jeito mais rápido de olhar as 3 faces com que você mexe, sem precisar
"traduzir" a planificação toda vez. As faces D, L e B ficam escondidas nessa
vista, como ficariam escondidas se você estivesse segurando o cubo de
verdade na mão. Aperte `v` de novo para voltar à planificação completa.

### 2.2. A janela 3D de verdade (tecla `j`, opcional)

Se o programa foi compilado com `make 3d` (precisa ter o raylib instalado —
veja a seção 1), a tecla `j` abre uma **janela separada com o cubo em 3D de
verdade**: você pode girar a câmera ao redor dele com as setas, e os giros
das faces são animados de verdade, não só um texto mudando.

Dentro da janela:

| Tecla | Ação |
|---|---|
| `u` `r` `f` | giro horário |
| `U` `R` `F` (shift) | giro anti-horário |
| setas | girar a câmera ao redor do cubo |
| `z` | desfazer |
| `c` | resetar para o estado resolvido |
| espaço | aplicar, com animação, a solução que você já calculou no console |
| `ESC` | fechar a janela e voltar ao console |

A janela é só para **jogar e ver** — ela não roda buscas novas. Se você
quer que a IA resolva, feche a janela (`ESC`), rode a busca no console
(`1`, `2`, `3` ou `m`) e depois abra a janela de novo (`j`) e aperte espaço
para ver a solução sendo aplicada em 3D.

> **Atenção:** se você mexer no cubo dentro da janela (qualquer giro
> manual, ou `z`) depois de já ter calculado uma solução, ela é descartada
> na hora — a mensagem "Solução pronta" some da tela, e o espaço não faz
> mais nada. Isso é de propósito: a solução foi calculada pra um cubo que
> não existe mais assim que você mexe nele, então aplicá-la aos trancos
> daria um resultado errado. Rode a busca de novo (no console) se precisar.

## 3. Jogando manualmente

Basta apertar uma tecla — **não precisa apertar Enter**. O programa reage na
hora.

| Tecla | O que faz |
|---|---|
| `u` | gira a face de cima (U) no sentido horário |
| `r` | gira a face da direita (R) no sentido horário |
| `f` | gira a face da frente (F) no sentido horário |
| `U` (shift+u) | gira a face de cima no sentido **anti-horário** |
| `R` (shift+r) | gira a face da direita no sentido **anti-horário** |
| `F` (shift+f) | gira a face da frente no sentido **anti-horário** |
| `z` | desfaz o último movimento que você fez |
| `c` | volta o cubo ao estado resolvido (mantém semente e histórico de tentativa) |
| `C` (shift+c) | **reinicia o programa inteiro** — cubo, histórico, última busca e semente voltam ao padrão, como se tivesse acabado de abrir |
| `v` | alterna entre a planificação (6 faces) e a vista de canto (3 faces) |

Repare que só existem três faces controláveis (U, R, F). Isso é de propósito
e não limita o que dá para fazer: como o cubo 2x2x2 não tem peças de centro,
girar as outras três faces (D, L, B) sempre dá no mesmo resultado que girar
essas três em outra ordem — é só uma questão de qual lado você está olhando.
Qualquer configuração do cubo pode ser alcançada usando só U, R e F.

## 4. Embaralhando o cubo

Aperte `e`. O programa vai pedir dois números pelo teclado (desta vez,
digite e aperte Enter, como normal):

1. **A semente**: qualquer número inteiro. É só uma "receita" que decide os
   movimentos aleatórios do embaralhamento.
2. **Quantos movimentos** (entre 1 e 40) embaralhar.

A vantagem da semente: **a mesma semente com o mesmo número de movimentos
sempre embaralha o cubo exatamente da mesma forma**, em qualquer computador.
Isso serve para comparar, por exemplo, quantos estados a Busca em Largura
visitou contra quantos o A* visitou, no *mesmo* cubo embaralhado — uma
comparação justa.

A tecla `c` volta o cubo direto ao estado resolvido, sem desfazer movimento
por movimento. Já `C` (shift+c) vai além: reinicia o programa inteiro — a
semente e o número de movimentos do embaralhamento também voltam ao padrão
(2024, 9 movimentos), como se você tivesse acabado de abrir o programa.

## 5. Pedindo para a IA resolver

Depois de embaralhar (ou a qualquer momento), aperte:

- `1` — resolve com **Busca em Largura**
- `2` — resolve com **Busca em Profundidade Limitada Iterativa**
- `3` — resolve com **A\***
- `m` — roda **as três de uma vez** no mesmo cubo e mostra uma tabela
  comparando quantos estados cada uma visitou e quanto tempo levou

O programa mostra "Rodando ..." e calcula a solução. Cubos pouco
embaralhados resolvem na hora; cubos com muitos movimentos de embaralhamento
podem levar alguns segundos (a Busca em Largura é a mais lenta das três —
o programa avisa quando isso pode acontecer).

Quando termina, a tela mostra:

- **Quantos movimentos** a solução tem;
- **Quantos estados a IA visitou** até encontrar a solução (um "estado" é
  uma configuração do cubo — visitar um estado é a IA examinar aquela
  configuração para ver se já é a solução);
- **Quanto tempo** levou;
- **A lista de movimentos** da solução, já como as teclas que você apertaria
  para refazê-la na mão — por exemplo: `r  u u  F  r r  u` (gire `r`, depois
  `u` duas vezes, depois `F`, depois `r` duas vezes, depois `u`).

Aperte `a` para **aplicar a solução no cubo**, um movimento por vez — a cada
tecla que você aperta, o próximo movimento da solução é feito e a tela
atualiza, até o cubo ficar resolvido. Durante esse passo a passo, o programa
também mostra a notação clássica de cubo mágico entre parênteses (`U`, `U2`,
`U'`...), caso você queira procurar essa mesma solução em algum lugar que
use essa notação.

Depois de rodar `m`, a solução do A\* fica pronta para aplicar com `a`, do
mesmo jeito.

## 6. O que cada estratégia de IA está fazendo (de forma simples)

Todas as três seguem a mesma ideia central: partem do cubo embaralhado,
testam se ele já está resolvido, e se não estiver, olham para os cubos
alcançáveis com mais um movimento, repetindo esse processo até achar a
solução. A diferença entre elas é **qual configuração elas escolhem olhar
primeiro**:

- **Busca em Largura**: examina os cubos alcançáveis com 1 movimento, depois
  todos os alcançáveis com 2 movimentos, depois 3, e assim por diante. Como
  nunca pula uma "camada" sem terminar a anterior, a primeira solução que
  encontra é garantidamente a mais curta possível. A desvantagem é que
  examina *muitos* cubos antes de chegar lá.

- **Busca em Profundidade Limitada Iterativa**: em vez de olhar camada por
  camada ao mesmo tempo, ela mergulha fundo num único caminho até um limite
  de movimentos, volta e tenta outro caminho. Ela repete tudo de novo com um
  limite maior (0, depois 1, depois 2, ...) até achar a solução — por isso
  também garante o caminho mais curto, mas refaz trabalho a cada rodada.

- **A\***: como as outras, mas usa uma "dica" (a **heurística**) para decidir
  qual cubo examinar primeiro: entre todos os cubos ainda não vistos, ela
  escolhe o que *parece* mais perto da solução. A dica usada aqui conta
  quantas peças ainda estão fora do lugar e estima, a partir disso, quantos
  movimentos no mínimo ainda faltam. Por escolher melhor por onde ir, o A\*
  costuma visitar muito menos cubos que as outras duas — nos testes feitos
  neste projeto, de 9 a 35 vezes menos.

Nenhuma das três "chuta" ou faz nada aleatório: dado o mesmo cubo embaralhado,
elas sempre encontram a mesma solução (mesmo número de movimentos), só que
gastando esforços bem diferentes para chegar lá.

## 7. Saindo

Aperte `q` a qualquer momento.

## 8. Perguntas frequentes

**As cores não aparecem, só apareceram códigos estranhos tipo `[41;97m`.**
Seu terminal não suporta cores ANSI. Use o PowerShell, o Windows Terminal ou
o terminal integrado do VS Code — todos suportam. O `cmd.exe` clássico do
Windows, em versões antigas, pode não suportar.

**Apertei uma tecla e nada aconteceu.**
Só as teclas listadas no menu fazem alguma coisa. Teclas fora da lista são
ignoradas silenciosamente.

**A Busca em Largura demorou muito.**
É esperado em cubos com muitos movimentos de embaralhamento (17 ou mais):
ela pode chegar a visitar milhões de estados. O A\* resolve o mesmo cubo
visitando muito menos.

**Posso repetir exatamente o mesmo embaralhamento depois?**
Sim — anote a semente e o número de movimentos que você usou, e digite os
mesmos valores da próxima vez que apertar `e`.

**Apertei `j` e não aconteceu nada / a tecla nem aparece no menu.**
O programa foi compilado sem raylib (`make`, sem o `3d`). Compile de novo
com `make 3d` (depois de instalar o raylib — ver seção 1) para ter a tecla.

**A janela 3D não abre e aparece um erro sobre `raylib.dll` ou `glfw3.dll`
faltando.**
Essas DLLs vêm com o raylib e ficam em `C:\msys64\ucrt64\bin`. Confira se
essa pasta está no PATH do Windows (é onde o `g++` também mora, então se
você já compila o projeto normalmente, ela já deve estar).
