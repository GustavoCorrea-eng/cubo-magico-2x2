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
