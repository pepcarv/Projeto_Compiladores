CC = gcc
CFLAGS = -Wall -Wno-unused-result -g -Og

ifeq ($(OS),Windows_NT)
    EXT = .exe
    EXEC =
else
    EXT =
    EXEC = ./
endif

COMPILADOR = $(EXEC)compilador$(EXT)
MEPA = $(EXEC)mepa$(EXT)

# aceita os testes dentro de Testes/ ou diretamente na raiz
PASTA_TESTES = $(firstword $(wildcard Testes/testesCadaUm) $(wildcard testesCadaUm))
CORRETOS = $(wildcard $(PASTA_TESTES)/corretos/*.txt)
ERROS = $(wildcard $(PASTA_TESTES)/erros/erro_*.txt)
ALVOS_CORRETOS = $(patsubst $(PASTA_TESTES)/corretos/%.txt,rodar-%,$(CORRETOS))
ALVOS_ERROS = $(patsubst $(PASTA_TESTES)/erros/%.txt,erro-%,$(ERROS))

.PHONY: all build help mepa erros clean
.NOTPARALLEL:

all: build

build:
	$(CC) $(CFLAGS) compilador.c -o compilador$(EXT)
	$(CC) $(CFLAGS) mepa.c -o mepa$(EXT)

help:
	@echo make build  - compila compilador.c e mepa.c
	@echo make mepa   - executa os testes da pasta corretos no MEPA
	@echo make erros  - executa os testes de erros lexicos, sintaticos e semanticos
	@echo make clean  - remove executaveis e arquivos gerados

mepa: build $(ALVOS_CORRETOS)

rodar-correto11_leitura:
	@echo ===== correto11_leitura =====
	@$(COMPILADOR) "$(PASTA_TESTES)/corretos/correto11_leitura.txt"
	@echo 5 | $(MEPA) -r -i mepa_cod.txt

rodar-correto18_leitura_vetor:
	@echo ===== correto18_leitura_vetor =====
	@$(COMPILADOR) "$(PASTA_TESTES)/corretos/correto18_leitura_vetor.txt"
	@echo 8 | $(MEPA) -r -i mepa_cod.txt

rodar-%:
	@echo ===== $* =====
	@$(COMPILADOR) "$(PASTA_TESTES)/corretos/$*.txt"
	@$(MEPA) -r -i mepa_cod.txt

erros: build $(ALVOS_ERROS)

erro-%:
	@echo ===== $* =====
	@$(COMPILADOR) "$(PASTA_TESTES)/erros/$*.txt" || echo erro esperado

clean:
ifeq ($(OS),Windows_NT)
	@if exist compilador.exe del /Q compilador.exe
	@if exist mepa.exe del /Q mepa.exe
	@if exist tokens_saida.txt del /Q tokens_saida.txt
	@if exist mepa_cod.txt del /Q mepa_cod.txt
	@if exist resultado_testes.txt del /Q resultado_testes.txt
else
	@rm -f compilador mepa tokens_saida.txt mepa_cod.txt resultado_testes.txt
endif