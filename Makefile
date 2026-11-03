CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -g -Og
SOURCE = compilador.c

ifeq ($(OS),Windows_NT)
TARGET = compilador.exe
PYTHON = python
else
TARGET = compilador
PYTHON = python3
endif

.PHONY: all build test clean
all: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

test: build
	$(PYTHON) testes/rodar_testes.py ./$(TARGET)

clean:
	$(PYTHON) -c "import os; [os.remove(f) for f in ['compilador', 'compilador.exe', 'tokens_saida.txt', 'tabela_simbolos.txt', 'resultado_testes.txt'] if os.path.exists(f)]"
