# Simulador de Cubo Magico 2x2x2
#
#   make          compila o programa basico (so texto, gera cubo2x2)
#   make 3d       compila TAMBEM com a janela 3D de verdade (raylib) -
#                 precisa ter o raylib instalado (veja abaixo)
#   make clean    apaga os executaveis
#
# No Windows (MSYS2 UCRT64) o "make" costuma ser instalado como
# "mingw32-make":  pacman -S mingw-w64-ucrt-x86_64-make
# Se preferir nao instalar nada, os comandos g++ do README fazem o mesmo.
#
# Para "make 3d", instale o raylib uma vez (MSYS2 UCRT64):
#   pacman -S mingw-w64-ucrt-x86_64-raylib

# Compilador de C++ usado para gerar os programas.
CXX     = g++
# Opcoes de compilacao: mostra todos os avisos (-Wall -Wextra),
# otimiza o codigo (-O2) e usa o padrao C++17.
CXXFLAGS = -Wall -Wextra -O2 -std=c++17

# Arquivos .cpp do programa basico (sem a janela 3D).
FONTES  = Cubo.cpp Busca.cpp Teclado.cpp main.cpp
# Cabecalhos (.hpp): se algum mudar, o make recompila o programa.
HEADERS = Cubo.hpp Fronteira.hpp Busca.hpp Teclado.hpp

# Ajustes por sistema operacional: no Windows o executavel termina em
# .exe e o raylib precisa de opengl32/gdi32/winmm; nos outros sistemas
# nao ha extensao e a lista de bibliotecas do raylib e a do Linux.
ifeq ($(OS),Windows_NT)
    EXE = .exe
    LIBS_3D = -lraylib -lopengl32 -lgdi32 -lwinmm
else
    EXE =
    LIBS_3D = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

# Alvo padrao (o que roda com so "make"): o programa basico.
all: cubo2x2$(EXE)

# Regra que gera o programa basico. Como depende dos fontes e cabecalhos,
# so recompila quando algum deles mudou. $@ e o nome do executavel.
cubo2x2$(EXE): $(FONTES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(FONTES) -o $@

# Atalho: "make 3d" gera o programa com a janela 3D (tecla j).
3d: cubo2x2-3d$(EXE)

# Regra da versao 3D: alem dos fontes basicos compila Janela3D.cpp,
# define COM_JANELA_3D (que liga a tecla j no main.cpp) e liga com o raylib.
cubo2x2-3d$(EXE): $(FONTES) Janela3D.cpp Janela3D.hpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -DCOM_JANELA_3D $(FONTES) Janela3D.cpp -o $@ $(LIBS_3D)

# "make clean" apaga os executaveis gerados.
clean:
	rm -f cubo2x2 cubo2x2.exe cubo2x2-3d cubo2x2-3d.exe

# Diz ao make que all, 3d e clean sao comandos e nao arquivos de verdade.
.PHONY: all 3d clean
