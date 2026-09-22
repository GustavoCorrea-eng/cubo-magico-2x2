# Simulador de Cubo Magico 2x2x2
#
#   make          compila o programa (gera cubo2x2)
#   make clean    apaga o executavel
#
# No Windows (MSYS2 UCRT64) o "make" costuma ser instalado como
# "mingw32-make":  pacman -S mingw-w64-ucrt-x86_64-make
# Se preferir nao instalar nada, o comando g++ do README faz o mesmo.

CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17

FONTES  = Cubo.cpp Fronteira.hpp Busca.cpp Teclado.cpp main.cpp
HEADERS = Cubo.hpp Fronteira.hpp Busca.hpp Teclado.hpp

ifeq ($(OS),Windows_NT)
    EXE = .exe
else
    EXE =
endif

all: cubo2x2$(EXE)

cubo2x2$(EXE): Cubo.cpp Busca.cpp Teclado.cpp main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) Cubo.cpp Busca.cpp Teclado.cpp main.cpp -o $@

clean:
	rm -f cubo2x2 cubo2x2.exe

.PHONY: all clean
