"""Testa o lexico, o sintatico e a tabela unica de simbolos."""

from pathlib import Path
import re
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parents[1]
FONTES = BASE / "testes"


def verificar(condicao, mensagem):
    if not condicao:
        raise AssertionError(mensagem)


def executar(binario, arquivo):
    with tempfile.TemporaryDirectory() as pasta:
        pasta = Path(pasta)
        processo = subprocess.run(
            [str(binario), str(arquivo)], cwd=pasta,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            encoding="utf-8", errors="replace", check=False
        )
        arquivo_tokens = pasta / "tokens_saida.txt"
        arquivo_simbolos = pasta / "tabela_simbolos.txt"
        verificar(arquivo_tokens.exists(), "tokens_saida.txt nao criado")
        verificar(arquivo_simbolos.exists(), "tabela_simbolos.txt nao criado")
        tokens = arquivo_tokens.read_text(encoding="utf-8")
        tabela = arquivo_simbolos.read_text(encoding="utf-8")
        verificar(processo.stdout == tokens, "saida do terminal difere do arquivo")
        verificar("PALAVRA_RESERVADA" not in tokens, "nome antigo de palavra reservada")
        verificar(not re.search(r"\d+# EOF\b", tokens), "EOF apareceu nos tokens")
        verificar("ERRO" not in processo.stderr, "erro inesperado no stderr")
        linhas = tabela.splitlines()
        verificar(linhas and linhas[0].startswith("INDICE\tCLASSE\tLEXEMA"),
                  "cabecalho da tabela incompleto")
        simbolos = {}
        chaves = set()
        for numero, linha in enumerate(linhas[1:], 1):
            partes = linha.split("\t")
            verificar(len(partes) == 7, "entrada incompleta na tabela")
            indice, classe, lexema = partes[:3]
            verificar(int(indice) == numero, "indices da tabela fora de ordem")
            verificar((classe, lexema) not in chaves, "simbolo duplicado")
            chaves.add((classe, lexema))
            simbolos[numero] = (classe, lexema)
        for linha in tokens.splitlines():
            match = re.fullmatch(r"(\d+)# (IDENTIFICADOR|STRING) \| (\d+)", linha)
            if match:
                _, nome, indice = match.groups()
                indice = int(indice)
                verificar(indice in simbolos, "token com indice fora da tabela")
                verificar(simbolos[indice][0] == nome, "ID/STRING aponta para outra classe")
        return processo, tokens, simbolos


def principal():
    binario = Path(sys.argv[1] if len(sys.argv) > 1 else BASE / "compilador").resolve()
    verificar(binario.is_file(), f"binario nao encontrado: {binario}")
    relatorio = ["BATERIA DE TESTES - PROJETO COMPILADORES", ""]
    aprovados = 0
    falhas = []

    for numero in range(1, 27):
        arquivo = FONTES / f"teste{numero}.txt"
        deve_falhar = numero in {15, 16, 20, 21, 24, 25, 26}
        categoria = "LEXICO" if numero in {15, 20, 21, 24, 25} else "SINTATICO"
        try:
            processo, tokens, simbolos = executar(binario, arquivo)
            if deve_falhar:
                verificar(processo.returncode != 0, "entrada invalida foi aceita")
                verificar(f"ERRO {categoria}" in tokens,
                          f"erro {categoria.lower()} nao identificado")
            else:
                verificar(processo.returncode == 0,
                          f"entrada valida recusada: {tokens[-250:]}")
            if numero in {17, 18}:
                verificar(any(lex == "auxiliar" or lex == "temporario"
                              for _, lex in simbolos.values()),
                          "variavel local nao registrada")
            if numero == 19:
                verificar("OP_RELACIONAL | <= " not in tokens, "formatacao inesperada")
                verificar("OP_RELACIONAL | <=\n" in tokens, "operador sem espaco nao reconhecido")
                verificar("ATRIBUICAO | <-" in tokens, "atribuicao sem espaco nao reconhecida")
            if numero == 22:
                indices_id = [int(i) for i in re.findall(r"# IDENTIFICADOR \| (\d+)", tokens)]
                indices_string = [int(i) for i in re.findall(r"# STRING \| (\d+)", tokens)]
                verificar(len(indices_id) > len(set(indices_id)), "IDs repetidos nao reutilizados")
                verificar(len(indices_string) > len(set(indices_string)),
                          "strings repetidas nao reutilizadas")
            estado = "OK"
            aprovados += 1
        except (AssertionError, OSError) as erro:
            estado = "FALHOU: " + str(erro)
            falhas.append(numero)
        linha = f"teste{numero:02d}.txt: {estado}"
        print(linha)
        relatorio.append(linha)

    relatorio.extend(["", f"Aprovados: {aprovados}/26", f"Falhas: {falhas}"])
    (BASE / "resultado_testes.txt").write_text("\n".join(relatorio) + "\n", encoding="utf-8")
    print(f"\n{aprovados}/26 aprovados. Relatorio: resultado_testes.txt")
    return 1 if falhas else 0


if __name__ == "__main__":
    sys.exit(principal())
