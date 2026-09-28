# Instalação da Janela 3D (raylib)

Este documento é só sobre o **extra opcional**: a janela 3D de verdade
(tecla `j`). Nada aqui é necessário para o resto do trabalho — o programa
básico (`make`, sem `3d`) compila e roda sem nenhum passo deste guia.

Se você só quer rodar o programa básico, veja o `MANUAL.md` em vez deste
arquivo.

## O que é preciso

1. **MSYS2** com a toolchain **UCRT64** (é o mesmo compilador `g++` que já
   compila o resto do projeto — se você já consegue rodar `make`, já tem
   isso).
2. O pacote **raylib**, instalado uma vez pelo gerenciador de pacotes do
   MSYS2 (`pacman`).

## Passo 1 — Confirme que já tem o MSYS2/UCRT64

Abra o PowerShell e rode:

```powershell
g++ --version
```

Se aparecer algo como `g++.exe (Rev...) ...` você já tem o compilador. Se
der "comando não encontrado", instale o MSYS2 primeiro (não é este trabalho
que precisa disso — é o ambiente de C++ como um todo):

1. Baixe o instalador em <https://www.msys2.org/> e rode-o (aceite os
   padrões).
2. Abra o menu Iniciar → **"MSYS2 UCRT64"** (não é "MSYS2 MSYS" nem "MSYS2
   MINGW64" — tem que ser especificamente o **UCRT64**, é a toolchain que
   este projeto usa).
3. Nessa janela, atualize os pacotes uma vez:
   ```bash
   pacman -Syu
   ```
   Se ele pedir para fechar a janela e reabrir para terminar a atualização,
   faça isso e rode `pacman -Syu` de novo.
4. Instale o compilador:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```
5. Adicione `C:\msys64\ucrt64\bin` ao PATH do Windows (Configurações →
   Sistema → Variáveis de Ambiente → `Path` → Novo), para o `g++` funcionar
   em qualquer terminal, não só dentro da janela do MSYS2.

## Passo 2 — Instale o raylib

Numa janela **"MSYS2 UCRT64"** (menu Iniciar), rode:

```bash
pacman -S mingw-w64-ucrt-x86_64-raylib
```

Isso também traz o **glfw** junto (dependência do raylib) automaticamente.
Confirme com `Y` se ele perguntar.

> **Se der "Connection timed out"**: os espelhos (mirrors) do MSYS2 às vezes
> ficam lentos. Rode o mesmo comando de novo — ele tenta outro espelho e
> geralmente completa na segunda ou terceira tentativa. Se continuar
> falhando, troque de rede (às vezes é o Wi-Fi/firewall da faculdade
> bloqueando) ou tente mais tarde.

### Conferindo que instalou certo

```bash
pacman -Q mingw-w64-ucrt-x86_64-raylib
```

Deve responder algo como `mingw-w64-ucrt-x86_64-raylib 5.5-2` (a versão pode
ser mais nova). Se não aparecer nada, a instalação falhou — repita o
Passo 2.

## Passo 3 — Compile com a janela 3D

De volta no PowerShell (ou no terminal que você já usa), dentro da pasta do
projeto:

```powershell
make 3d
```

Isso gera `cubo2x2-3d.exe`. Se preferir sem `make`, o comando equivalente
está no `README.md` (seção "Extra: janela 3D de verdade").

Se aparecer erro de `raylib.h: No such file or directory` ou parecido, o
raylib não foi encontrado — confira o Passo 2 e se `C:\msys64\ucrt64\bin`
está mesmo no PATH (rode `where g++` no PowerShell; o caminho deve começar
com `C:\msys64\ucrt64`).

## Passo 4 — Rode

```powershell
.\cubo2x2-3d.exe
```

Isso já abre o programa normal (texto). Dentro dele, aperte **`j`** para
abrir a janela 3D.

## Problemas comuns

**"O programa não abre / fecha na hora".**
Rode pelo terminal (não dando duplo-clique no `.exe` no Explorer) para ver
a mensagem de erro. O mais comum é faltar uma DLL — veja abaixo.

**Erro sobre `raylib.dll` ou `glfw3.dll` não encontrada.**
Essas DLLs ficam em `C:\msys64\ucrt64\bin` e são carregadas em tempo de
execução (o `.exe` não é standalone — ver `ESPECIFICACAO.md`, seção 10.1,
para o motivo técnico). Confirme que essa pasta está no PATH do Windows.
Se você compila o resto do projeto normalmente, ela já deve estar — mas se
não estiver, adicione do mesmo jeito do Passo 1.5.

**O antivírus/Windows Defender bloqueou o `.exe` ou a instalação.**
Raro, mas pode acontecer com executáveis recém-compilados. Adicione uma
exceção para a pasta do projeto, ou rode "Mais informações → Executar
assim mesmo" se o SmartScreen aparecer.

**`make: comando não encontrado`.**
Instale o `make` do MSYS2 (`pacman -S mingw-w64-ucrt-x86_64-make`, que
instala como `mingw32-make`) ou use o comando `g++` direto, listado no
`README.md`.

**Quero saber se vale a pena instalar tudo isso.**
Não é obrigatório: o trabalho inteiro (Estado, buscas, interface,
requisitos do enunciado) já funciona 100% sem a janela 3D. Ela é só um
extra visual — instale se quiser mostrar na arguição, mas não é bloqueador
para nada.
