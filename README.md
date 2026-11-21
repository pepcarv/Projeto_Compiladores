CC = gcc
CFLAGS = -Wall -Wno-unused-result -g -Og
TARGET = compilador.exe
SOURCE = compilador.c

.PHONY: all build test test-validos test-erros clean

all: build

build:
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

test: build
	@echo ======================================== > resultado_testes.txt
	@echo   BATERIA COMPLETA DE TESTES >> resultado_testes.txt
	@echo ======================================== >> resultado_testes.txt
	@echo. >> resultado_testes.txt
	@$(MAKE) test-validos
	@$(MAKE) test-erros
	@echo.
	@echo Testes finalizados.
	@echo Resultados salvos em resultado_testes.txt

test-validos:
	@echo ======================================== >> resultado_testes.txt
	@echo   TESTES QUE DEVEM SER ACEITOS >> resultado_testes.txt
	@echo ======================================== >> resultado_testes.txt
	@echo. >> resultado_testes.txt
	@for %%i in (2 3 4 5 6 7 8 9 10 11 12 13 14) do @echo ======================================== >> resultado_testes.txt & echo TESTE %%i - testes\teste%%i.txt >> resultado_testes.txt & echo ======================================== >> resultado_testes.txt & $(TARGET) testes\teste%%i.txt >> resultado_testes.txt 2>&1 & echo. >> resultado_testes.txt

test-erros:
	@echo ======================================== >> resultado_testes.txt
	@echo   TESTES QUE DEVEM GERAR ERRO >> resultado_testes.txt
	@echo ======================================== >> resultado_testes.txt
	@echo. >> resultado_testes.txt
	@for %%i in (15 16) do @echo ======================================== >> resultado_testes.txt & echo TESTE %%i - testes\teste%%i.txt >> resultado_testes.txt & echo ======================================== >> resultado_testes.txt & $(TARGET) testes\teste%%i.txt >> resultado_testes.txt 2>&1 & echo. >> resultado_testes.txt

clean:
	@if exist $(TARGET) del $(TARGET)
	@if exist tokens_saida.txt del tokens_saida.txt
	@if exist resultado_testes.txt del resultado_testes.txt