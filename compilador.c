/*
COMPILADORES - PROJETO - Fase 1: Analise Lexica e Analise Sintatica
Linguagem: MiniVisualg (Visualg Simplificado)

Integrantes do grupo:
  - 
  - Pedro Henrique Carvalho Pereira - 10418861

Compilacao (conforme especificado):
  gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador

Execucao:
  ./compilador <arquivo_fonte>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
SECAO 1: TIPOS E ESTRUTURAS (Figura 2 do descritivo - "Estrutura do Token
com a Union de Atributos")
*/

// Definicao dos nomes dos tokens (usados pelo Parser)
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID, // Identificadores (variaveis, funcoes)
    TOKEN_NUM_INT, // Numeros inteiros (Ex: 42)
    TOKEN_NUM_FLOAT, // Numeros reais (Ex: 3.14)
    TOKEN_OP_REL, // Operadores relacionais
    TOKEN_KEYWORD // Palavras reservadas
    // TODO: avaliar necessidade de tokens adicionais para cobrir toda a
    // gramatica da Etapa 1 (ex: operadores aritmeticos, operadores logicos,
    // delimitadores, string literal, atribuicao "<-", etc.), respeitando os
    // nomes ja sugeridos pelo descritivo e mantendo consistencia com a GLC
    // definida em etapa1_gramatica.txt
} TokenNome;

// Sub-codigos para especificar o atributo de operadores relacionais
typedef enum {
    OP_LT, // < (Less Than)
    OP_LE, // <= (Less or Equal)
    OP_EQ, // == (Equal) -- TODO: confirmar simbolo usado no MiniVisualg (Anexo I usa "=")
    OP_GT, // > (Greater Than)
    OP_GE // >= (Greater or Equal)
} OpRelAtributo;

// Estrutura do Token com a Union de Atributos (Figura 2)
typedef struct {
    TokenNome type; // Nome do token
    int line; // Para tratamento de erros

    // valor do atributo
    union {
        int table_index; // indice para Tabela de Simbolos
        int int_value; // Valor literal convertido
        double float_value; // Valor literal convertido
        OpRelAtributo op_code; // operador relacional especifico
    } attribute;
} Token;

// TODO: definir estrutura da Tabela de Simbolos (nao detalhada explicitamente
// na Figura 2, mas necessaria pois Token.attribute.table_index referencia
// indices dela). Deve armazenar pelo menos o lexema (nome) do identificador.

//SECAO 2: PROTOTIPOS

// ---- Analisador Lexico (Scanner) ----
// TODO: declarar variaveis globais/estado do scanner:
//   - char *buffer          (conteudo do arquivo fonte carregado em memoria)
//   - posicao atual de leitura no buffer
//   - linha atual (para reporte de erros)
//   - arquivo de saida dos tokens
Token obterToken(void);
void infoToken(Token t);

// ---- Analisador Sintatico (Parser) ----
// TODO: declarar variavel global do parser:
//   - Token lookahead (token atual sob analise, conforme Figura 1: "char lookahead")
void nextToken(void);

// TODO: declarar protótipos das funcoes de descida recursiva, uma para cada
// nao-terminal definido na gramatica da Etapa 1 (etapa1_gramatica.txt).
// Exemplos esperados a partir do Anexo I (ajustar nomes conforme a GLC final):
//   void analisar_programa(void);
//   void analisar_declaracoes(void);
//   void analisar_bloco_comandos(void);
//   void analisar_comando(void);
//   void analisar_atribuicao(void);
//   void analisar_condicional(void);       // se / senao / fimse
//   void analisar_repeticao_para(void);    // para / ate / passo / fimpara
//   void analisar_repeticao_enquanto(void);// enquanto / fimenquanto
//   void analisar_leitura(void);           // leia(...)
//   void analisar_escrita(void);           // escreva(...) / escreval(...)
//   void analisar_expressao(void);
//   void analisar_expressao_relacional(void);
//   void analisar_termo(void);
//   void analisar_fator(void);
//   void analisar_declaracao_procedimento(void);
//   void analisar_declaracao_funcao(void);
//   void analisar_chamada(void);

void erroLexico(int linha, const char *sequencia);
void erroSintatico(int linha, Token tokenIncorreto);

/*
SECAO 3: ANALISADOR LEXICO (SCANNER)
Interacao com o Parser conforme Figura 1: obterToken() / infoToken()
*/

// TODO (ETAPA 2): Implementar a leitura do arquivo fonte para um buffer em
// memoria (char *buffer), considerando que "todos os lexemas no codigo fonte
// estao separados por um espaco" (simplificacao dada pelo descritivo).

// TODO (ETAPA 2): Implementar tabela de palavras reservadas do MiniVisualg,
// extraidas dos exemplos do Anexo I, por exemplo:
//   algoritmo, var, inicio, fimalgoritmo, caractere, inteiro, real, logico,
//   vetor, de, escreva, escreval, leia, se, entao, senao, fimse, para, ate,
//   passo, faca, fimpara, enquanto, fimenquanto, procedimento,
//   fimprocedimento, funcao, fimfuncao, retorne, verdadeiro, falso, e, ou,
//   nao, mod
// TODO: confirmar lista completa e definitiva junto da Etapa 1 (GLC).

/*
TODO (ETAPA 2): obterToken()
Pseudocodigo baseado no fluxo descrito no PDF (Figura 1: SCANNER contem
char *buffer e obterToken()):
  1. Pular espacos/separadores ate proximo lexema (lexemas ja vem
     separados por espaco, conforme OBS do descritivo).
  2. Se fim do arquivo -> retornar Token{ type = TOKEN_EOF }.
  3. Identificar o proximo lexema:
     - Se comecar com letra -> ler ate separador; verificar se eh
       palavra reservada (TOKEN_KEYWORD) ou identificador (TOKEN_ID,
       registrar/consultar na Tabela de Simbolos -> attribute.table_index).
     - Se comecar com digito -> ler ate separador; se contiver '.'
       -> TOKEN_NUM_FLOAT (attribute.float_value), senao
       -> TOKEN_NUM_INT (attribute.int_value).
     - Se for operador relacional (<, <=, =, <>, >, >=) -> TOKEN_OP_REL
       com attribute.op_code (OP_LT, OP_LE, OP_EQ, OP_GT, OP_GE).
     - TODO: demais simbolos da linguagem (operadores aritmeticos,
       logicos, delimitadores, atribuicao "<-", string entre aspas,
       colchetes de vetor, virgula, dois-pontos, parenteses) conforme
       definido na Etapa 1.
  4. Se sequencia nao reconhecida -> chamar erroLexico(linha, sequencia)
     e encerrar o processo (conforme exigido no descritivo).
  5. Preencher Token.line com o numero da linha do lexema no arquivo fonte.
  6. Retornar o Token preenchido.
*/
Token obterToken(void) {
    // TODO: implementar conforme pseudocodigo acima
    Token t;
    // TODO: remover inicializacao provisoria abaixo apos implementacao real
    t.type = TOKEN_EOF;
    t.line = 0;
    return t;
}

/*
TODO (ETAPA 2): infoToken()
Conforme Figura 1, e responsavel por fornecer ao Parser as informacoes do
token corrente (usada pelo PARSER via infoToken()).
Deve tambem cobrir a exigencia de impressao no formato:
  "Numero da Linha do Atomo# NomeToken | Atributo"
  Exemplo: "11# IDENTIFICADOR | 1"  ou  "11# ID | 1"
TODO: garantir que a saida impressa na tela seja IDENTICA a saida gravada
no arquivo de tokens (mesma formatacao, mesma ordem).
*/
void infoToken(Token t) {
    // TODO: implementar formatacao e impressao/gravacao do token
    (void)t; // remover apos implementacao
}

/*
TODO (ETAPA 2): erroLexico()
Ao encontrar uma sequencia lexicamente invalida:
  1. Imprimir mensagem "ERRO LEXICO" junto com a linha do codigo fonte
     onde o erro foi localizado e a sequencia lexicamente errada.
  2. Finalizar todo o processo (compilacao deve parar).
*/
void erroLexico(int linha, const char *sequencia) {
    // TODO: implementar conforme pseudocodigo acima
    (void)linha;
    (void)sequencia;
}

/*
SECAO 4: ANALISADOR SINTATICO (PARSER)
Interacao com o Scanner conforme Figura 1: nextToken() chama obterToken()
*/

/*
TODO (ETAPA 3): nextToken()
Conforme Figura 1 (PARSER contem "char lookahead" e nextToken()):
  1. Chamar obterToken() (do analisador lexico) para obter o proximo token.
  2. Armazenar o resultado na variavel global lookahead do parser.
*/
void nextToken(void) {
    // TODO: implementar conforme pseudocodigo acima
}

/*
TODO (ETAPA 3): Implementar analise descendente (recursiva) com base na
Gramatica Livre de Contexto definida na Etapa 1 (etapa1_gramatica.txt).

Estrutura geral esperada (a partir do "ESQUELETO" do Anexo I):

  analisar_programa():
    - esperar TOKEN_KEYWORD "algoritmo"
    - esperar literal de string (nome do algoritmo)
    - esperar TOKEN_KEYWORD "var"
    - chamar analisar_declaracoes() (pode ser vazio, conforme exemplo
      "PrimeiroPasso": // A area de variaveis esta vazia)
    - esperar TOKEN_KEYWORD "inicio"
    - chamar analisar_bloco_comandos()
    - esperar TOKEN_KEYWORD "fimalgoritmo"

  analisar_declaracoes(): (secao VARIAVEIS do Anexo I)
    - repetir enquanto houver identificador seguido de ":":
        lista de identificadores separados por virgula
        ":" tipo (caractere | inteiro | real | logico | vetor[n..m] de tipo)

  analisar_bloco_comandos():
    - repetir analisar_comando() ate encontrar palavra-chave de
      fechamento de bloco (fimalgoritmo / senao / fimse / fimpara /
      fimenquanto / fimprocedimento / fimfuncao)

  analisar_comando():
    - despachar conforme o lookahead para uma das seguintes (Anexo I):
        atribuicao        (id <- expressao)
        leia(id)
        escreva(lista_expressoes) / escreval(lista_expressoes)
        se (...) entao ... [senao ...] fimse
        para id de expr ate expr [passo expr] faca ... fimpara
        enquanto (...) faca ... fimenquanto
        retorne expressao        (dentro de funcao)
        chamada de procedimento/funcao

  analisar_expressao() / analisar_expressao_relacional() / analisar_termo()
  / analisar_fator():
    - hierarquia classica de expressoes, cobrindo:
        operadores relacionais (=, <>, <, <=, >, >=)   [OPERADORES RELACIONAIS]
        operadores logicos (E, OU)                      [OPERADORES LOGICOS]
        operadores aritmeticos (+, -, *, /, \, MOD)      [OPERADORES ARITMETICOS]
        literais (numero inteiro, numero real, string, verdadeiro/falso)
        identificador (variavel simples ou indexada: id[expr])
        chamada de funcao dentro de expressao (ex: eh_par(num))
        parenteses para agrupamento

  analisar_declaracao_procedimento() / analisar_declaracao_funcao():
    - conforme secao PROCEDIMENTOS e FUNCOES do Anexo I:
        procedimento nome [ ( lista_parametros ) ] ... fimprocedimento
        funcao nome ( lista_parametros ) : tipo ... fimfuncao
      (lembrar: procedimento sem parametros NAO usa parenteses, nem na
      declaracao nem na chamada)

TODO: erroSintatico()
  Ao encontrar um token que nao corresponde ao esperado pela gramatica:
    1. Imprimir mensagem "ERRO SINTATICO" junto com o token incorreto e a
       linha do codigo fonte correspondente.
    2. Finalizar todo o processo.
*/
void erroSintatico(int linha, Token tokenIncorreto) {
    // TODO: implementar conforme pseudocodigo acima
    (void)linha;
    (void)tokenIncorreto;
}

//SECAO 5: FUNCAO PRINCIPAL


/*
TODO (ETAPA 2/3): main()
  1. Validar argumentos de linha de comando: ./compilador <arquivo_fonte>
     Se nome do arquivo nao informado -> mensagem de uso e encerrar.
  2. Abrir o arquivo fonte informado.
  3. Carregar conteudo para o buffer do analisador lexico.
  4. Abrir/preparar arquivo de saida dos tokens.
  5. Chamar nextToken() para obter o primeiro lookahead.
  6. Chamar a funcao de entrada do parser (ex: analisar_programa()).
  7. Fechar arquivos.
  8. Retornar 0 em caso de sucesso (exigencia: warning/retorno != 0 gera
     desconto de 1.0 ponto por ocorrencia).
*/
int main(int argc, char *argv[]) {
    // TODO: implementar conforme pseudocodigo acima
    (void)argc;
    (void)argv;

    return 0;
}