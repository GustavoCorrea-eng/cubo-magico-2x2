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

CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17

FONTES  = Cubo.cpp Busca.cpp Teclado.cpp main.cpp
HEADERS = Cubo.hpp Fronteira.hpp Busca.hpp Teclado.hpp

ifeq ($(OS),Windows_NT)
    EXE = .exe
    LIBS_3D = -lraylib -lopengl32 -lgdi32 -lwinmm
else
    EXE =
    LIBS_3D = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

all: cubo2x2$(EXE)

cubo2x2$(EXE): $(FONTES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(FONTES) -o $@

3d: cubo2x2-3d$(EXE)

cubo2x2-3d$(EXE): $(FONTES) Janela3D.cpp Janela3D.hpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -DCOM_JANELA_3D $(FONTES) Janela3D.cpp -o $@ $(LIBS_3D)

clean:
	rm -f cubo2x2 cubo2x2.exe cubo2x2-3d cubo2x2-3d.exe

.PHONY: all 3d clean
