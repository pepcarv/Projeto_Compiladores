/*
COMPILADORES - PROJETO - Fase 1: Analise Lexica e Analise Sintatica
Linguagem: MiniVisualg (Visualg Simplificado)

Integrantes do grupo:
  - Mateus Ribeiro Cerqueira - 10443901
  - Pedro Henrique Carvalho Pereira - 10418861

Compilacao:
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
    TOKEN_KEYWORD, // Palavras reservadas
    // TODO: avaliar necessidade de tokens adicionais para cobrir toda a
    // gramatica da Etapa 1 (ex: operadores aritmeticos, operadores logicos,
    // delimitadores, string literal, atribuicao "<-", etc.), respeitando os
    // nomes ja sugeridos pelo descritivo e mantendo consistencia com a GLC
    // definida em etapa1_gramatica.txt
    TOKEN_STRING,
    TOKEN_OP_ARIT,
    TOKEN_OP_LOG,
    TOKEN_ASSIGN,
    TOKEN_DELIM
} TokenNome;

// Sub-codigos para especificar o atributo de operadores relacionais
typedef enum {
    OP_LT, // < (Less Than)
    OP_LE, // <= (Less or Equal)
    OP_EQ, // == (Equal) -- TODO: confirmar simbolo usado no MiniVisualg (Anexo I usa "=")
    OP_GT, // > (Greater Than)
    OP_GE, // >= (Greater or Equal)
    OP_NE /* <> Not Equal */
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

/* Tabela de Simbolos: guarda o texto (lexema) de identificadores, strings,
   palavras reservadas e operadores/delimitadores, referenciados pelos
   tokens atraves de attribute.table_index. */
#define SYM_CAP_INICIAL 64
static char **symtab = NULL;
static int symCount = 0;
static int symCap = 0;

static int addSymbol(const char *s) {
  if (symCount >= symCap) {
    symCap = (symCap == 0) ? SYM_CAP_INICIAL : symCap * 2;
    symtab = (char **) realloc(symtab, (size_t) symCap * sizeof(char *));
  }
  symtab[symCount] = (char *) malloc(strlen(s) + 1);
  strcpy(symtab[symCount], s);
  symCount++;
  return symCount - 1;
}

/* Tabela de identificadores: usada apenas para reconhecer repeticoes.
   O token guarda somente o NUMERO do identificador (1, 2, 3...). */
static char **idtab = NULL;
static int idCount = 0;
static int idCap = 0;

// Retorna o numero (1-based) do identificador, cadastrando se for novo
static int findOrAddIdentifier(const char *s) {
  for (int i = 0; i < idCount; i++) {
    if (strcmp(idtab[i], s) == 0) return i + 1;
  }
  if (idCount >= idCap) {
    idCap = (idCap == 0) ? SYM_CAP_INICIAL : idCap * 2;
    idtab = (char **) realloc(idtab, (size_t) idCap * sizeof(char *));
  }
  idtab[idCount] = (char *) malloc(strlen(s) + 1);
  strcpy(idtab[idCount], s);
  idCount++;
  return idCount;
}

//SECAO 2: PROTOTIPOS

// ---- Analisador Lexico (Scanner) ----
// TODO: declarar variaveis globais/estado do scanner:
//   - char *buffer          (conteudo do arquivo fonte carregado em memoria)
//   - posicao atual de leitura no buffer
//   - linha atual (para reporte de erros)
//   - arquivo de saida dos tokens

static char *buffer = NULL;   /* conteudo do arquivo fonte carregado em memoria */
static long bufLen = 0;
static long bufPos = 0;
static int linhaAtual = 1;
static FILE *arqSaida = NULL; /* arquivo de saida dos tokens */


Token obterToken(void);
void infoToken(Token t);

// ---- Analisador Sintatico (Parser) ----
// TODO: declarar variavel global do parser:
//   - Token lookahead (token atual sob analise, conforme Figura 1: "char lookahead")
static Token lookahead;
void nextToken(void);

// TODO: declarar protótipos das funcoes de descida recursiva, uma para cada
// nao-terminal definido na gramatica da Etapa 1 (etapa1_gramatica.txt).
// Exemplos esperados a partir do Anexo I (ajustar nomes conforme a GLC final):

// ---- Analisador Sintatico (Parser) ----

void consome(TokenNome tipo, const char *lexema);
void analisar_programa(void);
void analisar_declaracoes(void);
void analisar_tipo(void);
void analisar_bloco_comandos(void);
int eh_fim_bloco(void);
void analisar_comando(void);
void analisar_leitura(void);
void analisar_escrita(void);
void analisar_condicional(void);
void analisar_repeticao_para(void);
void analisar_repeticao_enquanto(void);
void analisar_lista_argumentos(void);
void analisar_expressao(void);
void analisar_expr_logica(void);
void analisar_expr_relacional(void);
void analisar_expr_aritmetica(void);
void analisar_termo(void);
void analisar_fator(void);
void analisar_subrotinas(void);
void analisar_declaracao_procedimento(void);
void analisar_declaracao_funcao(void);
void analisar_lista_parametros(void);
void erroSintatico(int linha, Token tokenIncorreto);

void erroLexico(int linha, const char *sequencia);
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


// Palavras reservadas do MiniVisualg, levantadas a partir do Anexo I
static const char *palavrasReservadas[] = {
  "algoritmo", "var", "inicio", "fimalgoritmo",
  "caractere", "inteiro", "real", "logico", "vetor", "de",
  "escreva", "escreval", "leia",
  "se", "entao", "senao", "fimse",
  "para", "ate", "passo", "faca", "fimpara",
  "enquanto", "fimenquanto",
  "procedimento", "fimprocedimento",
  "funcao", "fimfuncao", "retorne",
  "verdadeiro", "falso",
  "E", "OU", "mod", "MOD"
};


static const int totalPalavrasReservadas = (int) (sizeof(palavrasReservadas) / sizeof(palavrasReservadas[0]));





static int ehPalavraReservada(const char *lexema) {
  for (int i = 0; i < totalPalavrasReservadas; i++) {
    if (strcmp(palavrasReservadas[i], lexema) == 0) return 1;
  }
  return 0;
}



static int fimDoArquivo(void) {
  return bufPos >= bufLen;
}

static char charAtual(void) {
  if (fimDoArquivo()) return '\0';
  return buffer[bufPos];
}


static char proximoChar(void) {
  if (bufPos + 1 >= bufLen) return '\0';
  return buffer[bufPos + 1];
}



static void avancarChar(void) {
  if (fimDoArquivo()) return;
  if (buffer[bufPos] == '\n') linhaAtual++;
  bufPos++;
}

// pular espacos em branco e comentarios (// ate o fim da linha)
static void pularEspacosEComentarios(void) {
  //loop infinito com for só pra conter o whiule
  for (;;) {
    while (!fimDoArquivo() && isspace((unsigned char) charAtual())) {
      avancarChar();
    }
    if (!fimDoArquivo() && charAtual() == '/' && proximoChar() == '/') {
      while (!fimDoArquivo() && charAtual() != '\n') avancarChar();
      continue;
    }
    break;
  }
}

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
  Token t;

  pularEspacosEComentarios();
  t.line = linhaAtual;

  if (fimDoArquivo()) {
    t.type = TOKEN_EOF;
    return t;
  }

  char c = charAtual();
  char lex[256];
  int n;

  // Identificador ou palavra reservada 
  if (isalpha((unsigned char) c) || c == '_') {
    n = 0;




    while (!fimDoArquivo() && (isalnum((unsigned char) charAtual()) || charAtual() == '_')) {
      if (n < 255) lex[n++] = charAtual();
      //printf("%d",n);
      avancarChar();
    }
    lex[n] = '\0';



    if (ehPalavraReservada(lex)) {
      t.type = TOKEN_KEYWORD;
      t.attribute.table_index = addSymbol(lex);
    } 
    
    
    else {
      t.type = TOKEN_ID;
      t.attribute.table_index = findOrAddIdentifier(lex);
    }

    
    return t;
  }

  // Numero inteiro ou real
  if (isdigit((unsigned char) c)) {
    n = 0;
    while (!fimDoArquivo() && isdigit((unsigned char) charAtual())) {
      if (n < 255) lex[n++] = charAtual();
      avancarChar();
    }
    if (!fimDoArquivo() && charAtual() == '.' && isdigit((unsigned char) proximoChar())) {
      if (n < 255) lex[n++] = charAtual();
      avancarChar();
      while (!fimDoArquivo() && isdigit((unsigned char) charAtual())) {
        if (n < 255) lex[n++] = charAtual();
        avancarChar();
      }
      lex[n] = '\0';
      t.type = TOKEN_NUM_FLOAT;
      t.attribute.float_value = atof(lex);
    } else {
      lex[n] = '\0';
      t.type = TOKEN_NUM_INT;
      t.attribute.int_value = atoi(lex);
    }
    return t;
  }

  // Cadeia de caracteres literal
  if (c == '"') {
    avancarChar(); // aspa abre
    n = 0;
    while (!fimDoArquivo() && charAtual() != '"' && charAtual() != '\n') {
      if (n < 255) lex[n++] = charAtual();
      avancarChar();
    }
    if (fimDoArquivo() || charAtual() != '"') {
      lex[n] = '\0';
      erroLexico(t.line, lex);
    }
    avancarChar(); // fecha aspa 
    lex[n] = '\0';
    t.type = TOKEN_STRING;
    t.attribute.table_index = addSymbol(lex);
    return t;
  }

  // operadores relacionais e atribuicao
  if (c == '<') {
    avancarChar();
    if (charAtual() == '-') {
      avancarChar();
      t.type = TOKEN_ASSIGN;
      t.attribute.table_index = addSymbol("<-");
    } else if (charAtual() == '=') {
      avancarChar();
      t.type = TOKEN_OP_REL;
      t.attribute.op_code = OP_LE;
    } else if (charAtual() == '>') {
      avancarChar();
      t.type = TOKEN_OP_REL;
      t.attribute.op_code = OP_NE;
    } else {
      t.type = TOKEN_OP_REL;
      t.attribute.op_code = OP_LT;
    }
    return t;
  }




  if (c == '>') {
    avancarChar();
    if (charAtual() == '=') {
      avancarChar();
      t.type = TOKEN_OP_REL;
      t.attribute.op_code = OP_GE;
    } else {
      t.type = TOKEN_OP_REL;
      t.attribute.op_code = OP_GT;
    }
    return t;
  }
  if (c == '=') {
    avancarChar();
    t.type = TOKEN_OP_REL;
    t.attribute.op_code = OP_EQ;
    return t;
  }







  // Operadores aritmeticos
  if (c == '+' || c == '-' || c == '*' || c == '/' || c == '\\') {
    avancarChar();
    lex[0] = c;
    lex[1] = '\0';
    t.type = TOKEN_OP_ARIT;
    t.attribute.table_index = addSymbol(lex);
    return t;
  }



  // Intervalo de vetor ".."
  if (c == '.' && proximoChar() == '.') {
    avancarChar();
    avancarChar();
    t.type = TOKEN_DELIM;
    t.attribute.table_index = addSymbol("..");
    return t;
  }



  // Delimitadores
  if (c == '(' || c == ')' || c == '[' || c == ']' || c == ',' || c == ':') {
    avancarChar();
    lex[0] = c;
    lex[1] = '\0';
    t.type = TOKEN_DELIM;
    t.attribute.table_index = addSymbol(lex);
    return t;
  }

  // erro - encerra o processo
  n = 0;
  while (!fimDoArquivo() && !isspace((unsigned char) charAtual()) && n < 255) {
      lex[n++] = charAtual();
      //printf("%d",n);
      avancarChar();
  }
  if (n == 0 && !fimDoArquivo()) {
    //printf("%d",n);
      lex[n++] = charAtual();
      avancarChar();
  }
  lex[n] = '\0';
  erroLexico(t.line, lex);
  t.type = TOKEN_EOF; //erroLexico encerra o processo
  return t;
}

//melhorar para SC facil pra qlqr tipo
static const char *nomeDoToken(TokenNome type) {
  switch (type) {
    case TOKEN_EOF: return "EOF";
    
    case TOKEN_ID: return "IDENTIFICADOR";
    
    case TOKEN_NUM_INT: return "NUM_INTEIRO";
    
    case TOKEN_NUM_FLOAT: return "NUM_REAL";
    
    case TOKEN_OP_REL: return "OP_RELACIONAL";
    
    case TOKEN_KEYWORD: return "PALAVRA_RESERVADA";
    
    case TOKEN_STRING: return "STRING";
    
    case TOKEN_OP_LOG: return "OP_LOGICO";

    case TOKEN_OP_ARIT: return "OP_ARITMETICO";
    
    case TOKEN_ASSIGN: return "ATRIBUICAO";
    
    case TOKEN_DELIM: return "DELIMITADOR";
  }
  return "DESCONHECIDO";
}


static const char *nomeOpRel(OpRelAtributo op) {
    switch (op) {
      case OP_LT: return "<";
      
      case OP_LE: return "<=";
      
      case OP_EQ: return "=";
      
      
      case OP_NE: return "<>";
      
      case OP_GT: return ">";
      
      
      case OP_GE: return ">=";
    }
    return "?";
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
  char linhaFormatada[512];

  if (t.type == TOKEN_EOF) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s\n", t.line, nomeDoToken(t.type));
  } 
  else if (t.type == TOKEN_NUM_INT) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s | %d\n", t.line, nomeDoToken(t.type), t.attribute.int_value);
  } 
  else if (t.type == TOKEN_NUM_FLOAT) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s | %g\n", t.line, nomeDoToken(t.type), t.attribute.float_value);
  } 
  else if (t.type == TOKEN_OP_REL) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s | %s\n", t.line, nomeDoToken(t.type), nomeOpRel(t.attribute.op_code));
  } 
  else if (t.type == TOKEN_ID) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s | %d\n", t.line, nomeDoToken(t.type), t.attribute.table_index);
  }
  else {
      // ID, KEYWORD, STRING, OP_ARIT, ASSIGN, DELIM 
      // guardado na tabela de simbolos
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s | %s\n", t.line, nomeDoToken(t.type), symtab[t.attribute.table_index]);
  }

  printf("%s", linhaFormatada);
  if (arqSaida != NULL) fputs(linhaFormatada, arqSaida);
}

/*
TODO (ETAPA 2): erroLexico()
Ao encontrar uma sequencia lexicamente invalida:
  1. Imprimir mensagem "ERRO LEXICO" junto com a linha do codigo fonte
     onde o erro foi localizado e a sequencia lexicamente errada.
  2. Finalizar todo o processo (compilacao deve parar).
*/
void erroLexico(int linha, const char *sequencia) {
  printf("ERRO LEXICO na linha %d: sequencia invalida \"%s\"\n", linha, sequencia);
  
  
  if (arqSaida != NULL) {
    fprintf(arqSaida, "ERRO LEXICO na linha %d: sequencia invalida \"%s\"\n", linha, sequencia);
    fclose(arqSaida);
  }
  exit(1);
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
  lookahead = obterToken();
  infoToken(lookahead);
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


void consome(TokenNome tipo, const char *lexema) {
  if (lookahead.type != tipo)
    erroSintatico(lookahead.line, lookahead);

  if (lexema != NULL) {
    if (strcmp(symtab[lookahead.attribute.table_index], lexema) != 0) 
      erroSintatico(lookahead.line, lookahead);
  }

  nextToken();
}
void analisar_programa(void) {
  consome(TOKEN_KEYWORD, "algoritmo");
  consome(TOKEN_STRING, NULL);
  analisar_subrotinas();
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "var") == 0) {
    consome(TOKEN_KEYWORD, "var");
    analisar_declaracoes();
  }
  consome(TOKEN_KEYWORD, "inicio");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimalgoritmo");
}
void analisar_declaracoes(void) {
  while (lookahead.type == TOKEN_ID) {
    consome(TOKEN_ID, NULL);
    while (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
      consome(TOKEN_DELIM, ",");
      consome(TOKEN_ID, NULL);
    }
    consome(TOKEN_DELIM, ":");
    analisar_tipo();
  }
}
void analisar_tipo(void) {
  if (lookahead.type != TOKEN_KEYWORD) erroSintatico(lookahead.line, lookahead);

  if (strcmp(symtab[lookahead.attribute.table_index], "caractere") == 0) {
    consome(TOKEN_KEYWORD, "caractere");
  } else if (strcmp(symtab[lookahead.attribute.table_index], "inteiro") == 0) {
    consome(TOKEN_KEYWORD, "inteiro");
  } else if (strcmp(symtab[lookahead.attribute.table_index], "real") == 0) {
    consome(TOKEN_KEYWORD, "real");
  } else if (strcmp(symtab[lookahead.attribute.table_index], "logico") == 0) {
    consome(TOKEN_KEYWORD, "logico");
  } else if (strcmp(symtab[lookahead.attribute.table_index], "vetor") == 0) {
    consome(TOKEN_KEYWORD, "vetor");
    consome(TOKEN_DELIM, "[");
    consome(TOKEN_NUM_INT, NULL);
    consome(TOKEN_DELIM, "..");
    consome(TOKEN_NUM_INT, NULL);
    consome(TOKEN_DELIM, "]");
    consome(TOKEN_KEYWORD, "de");
    analisar_tipo();
  } else {
    erroSintatico(lookahead.line, lookahead);
  }
}
void analisar_bloco_comandos(void) {
    while (!eh_fim_bloco()) analisar_comando();
}
int eh_fim_bloco(void) {
    if (lookahead.type != TOKEN_KEYWORD) return 0;
    const char *lexema = symtab[lookahead.attribute.table_index];
    return strcmp(lexema, "fimalgoritmo") == 0 ||
           strcmp(lexema, "senao") == 0 ||
           strcmp(lexema, "fimse") == 0 ||
           strcmp(lexema, "fimpara") == 0 ||
           strcmp(lexema, "fimenquanto") == 0 ||
           strcmp(lexema, "fimprocedimento") == 0 ||
           strcmp(lexema, "fimfuncao") == 0;
}
void analisar_comando(void) {
  if (lookahead.type == TOKEN_KEYWORD) {
    if (strcmp(symtab[lookahead.attribute.table_index], "escreva") == 0 || strcmp(symtab[lookahead.attribute.table_index], "escreval") == 0) {
      analisar_escrita();
    } else if (strcmp(symtab[lookahead.attribute.table_index], "leia") == 0) {
      analisar_leitura();
    } else if (strcmp(symtab[lookahead.attribute.table_index], "se") == 0) {
      analisar_condicional();
    } else if (strcmp(symtab[lookahead.attribute.table_index], "para") == 0) {
      analisar_repeticao_para();
    } else if (strcmp(symtab[lookahead.attribute.table_index], "enquanto") == 0) {
      analisar_repeticao_enquanto();
    } else if (strcmp(symtab[lookahead.attribute.table_index], "retorne") == 0) {
      consome(TOKEN_KEYWORD, "retorne");
      analisar_expressao();
    } else erroSintatico(lookahead.line, lookahead);
  } else if (lookahead.type == TOKEN_ID) {
    consome(TOKEN_ID, NULL);
    if (lookahead.type == TOKEN_ASSIGN) {
      consome(TOKEN_ASSIGN, NULL);
      analisar_expressao();
    } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
      consome(TOKEN_DELIM, "(");
      analisar_lista_argumentos();
      consome(TOKEN_DELIM, ")");
    } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
      consome(TOKEN_DELIM, "[");
      analisar_expressao();
      consome(TOKEN_DELIM, "]");
      consome(TOKEN_ASSIGN, NULL);
      analisar_expressao();
    }
  } else erroSintatico(lookahead.line, lookahead);
}
void analisar_leitura(void) {
    consome(TOKEN_KEYWORD, "leia");
    consome(TOKEN_DELIM, "(");
    consome(TOKEN_ID, NULL);
    if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
      consome(TOKEN_DELIM, "[");
      analisar_expressao();
      consome(TOKEN_DELIM, "]");
    }
    consome(TOKEN_DELIM, ")");
}
void analisar_escrita(void) {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "escreva") == 0) {
      consome(TOKEN_KEYWORD, "escreva");
    } else if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "escreval") == 0) {
      consome(TOKEN_KEYWORD, "escreval");
    } else {
      erroSintatico(lookahead.line, lookahead);
    }
    consome(TOKEN_DELIM, "(");
    analisar_lista_argumentos();
    consome(TOKEN_DELIM, ")");
}
void analisar_lista_argumentos(void) {
    analisar_expressao();
    while (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
      consome(TOKEN_DELIM, ",");
      analisar_expressao();
    }
}
void analisar_expressao(void) {
  analisar_expr_logica();
}
void analisar_expr_logica(void) {
  analisar_expr_relacional();
  while (lookahead.type == TOKEN_KEYWORD && (strcmp(symtab[lookahead.attribute.table_index], "E") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "OU") == 0)) {
    if (strcmp(symtab[lookahead.attribute.table_index], "OU") == 0) {
      consome(TOKEN_KEYWORD, "OU");
    } else {
      consome(TOKEN_KEYWORD, "E");
    }
      analisar_expr_relacional();
  }
}
void analisar_expr_relacional(void) {
  analisar_expr_aritmetica();
  if (lookahead.type == TOKEN_OP_REL) {
    consome(TOKEN_OP_REL, NULL);
    analisar_expr_aritmetica();
  }
}
void analisar_expr_aritmetica(void) {
  analisar_termo();
  while (lookahead.type == TOKEN_OP_ARIT && (strcmp(symtab[lookahead.attribute.table_index], "+") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "-") == 0)) {
    if (strcmp(symtab[lookahead.attribute.table_index], "+") == 0) {
      consome(TOKEN_OP_ARIT, "+");
    } else {
      consome(TOKEN_OP_ARIT, "-");
    }
    analisar_termo();
  }
}
void analisar_termo(void) {
  analisar_fator();
  while ((lookahead.type == TOKEN_OP_ARIT && 
    (strcmp(symtab[lookahead.attribute.table_index], "*") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "/") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "\\") == 0)) 
    ||
    (lookahead.type == TOKEN_KEYWORD && 
    (strcmp(symtab[lookahead.attribute.table_index], "MOD") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "mod") == 0))) {

    if (lookahead.type == TOKEN_OP_ARIT) {
      if (strcmp(symtab[lookahead.attribute.table_index], "*") == 0) {
        consome(TOKEN_OP_ARIT, "*");
      } else if (strcmp(symtab[lookahead.attribute.table_index], "/") == 0) {
        consome(TOKEN_OP_ARIT, "/");
      } else {
        consome(TOKEN_OP_ARIT, "\\");
      }
    } else {
      if (strcmp(symtab[lookahead.attribute.table_index], "MOD") == 0) {
        consome(TOKEN_KEYWORD, "MOD");
      } else {
        consome(TOKEN_KEYWORD, "mod");
      }
    }
    analisar_fator();
  }
}
void analisar_fator(void) {
  if (lookahead.type == TOKEN_OP_ARIT && (strcmp(symtab[lookahead.attribute.table_index], "+") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "-") == 0)) {
    if (strcmp(symtab[lookahead.attribute.table_index], "+") == 0) {
      consome(TOKEN_OP_ARIT, "+");
    } else {
      consome(TOKEN_OP_ARIT, "-");
    }
    analisar_fator();
    return;
  }
  
  if (lookahead.type == TOKEN_NUM_INT) {
    consome(TOKEN_NUM_INT, NULL);
  } else if (lookahead.type == TOKEN_NUM_FLOAT) {
    consome(TOKEN_NUM_FLOAT, NULL);
  } else if (lookahead.type == TOKEN_STRING) {
    consome(TOKEN_STRING, NULL);
  } else if (lookahead.type == TOKEN_KEYWORD && (strcmp(symtab[lookahead.attribute.table_index], "verdadeiro") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "falso") == 0)) {
    if (strcmp(symtab[lookahead.attribute.table_index], "verdadeiro") == 0) {
      consome(TOKEN_KEYWORD, "verdadeiro");
    } else {
      consome(TOKEN_KEYWORD, "falso");
    }
  } else if (lookahead.type == TOKEN_ID) {
    consome(TOKEN_ID, NULL);
    if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
      consome(TOKEN_DELIM, "(");
      analisar_lista_argumentos();
      consome(TOKEN_DELIM, ")");
    } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
      consome(TOKEN_DELIM, "[");
      analisar_expressao();
      consome(TOKEN_DELIM, "]");
    }
  } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
    consome(TOKEN_DELIM, "(");
    analisar_expressao();
    consome(TOKEN_DELIM, ")");
  } else {
    erroSintatico(lookahead.line, lookahead);
  }
}
void analisar_condicional(void) {
  consome(TOKEN_KEYWORD, "se");
  consome(TOKEN_DELIM, "(");
  analisar_expressao();
  consome(TOKEN_DELIM, ")");
  consome(TOKEN_KEYWORD, "entao");
  analisar_bloco_comandos();
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "senao") == 0) {
    consome(TOKEN_KEYWORD, "senao");
    analisar_bloco_comandos();
  }
  consome(TOKEN_KEYWORD, "fimse");
}
void analisar_repeticao_enquanto(void) {
  consome(TOKEN_KEYWORD, "enquanto");
  consome(TOKEN_DELIM, "(");
  analisar_expressao();
  consome(TOKEN_DELIM, ")");
  consome(TOKEN_KEYWORD, "faca");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimenquanto");
}
void analisar_repeticao_para(void) {
  consome(TOKEN_KEYWORD, "para");
  consome(TOKEN_ID, NULL);
  consome(TOKEN_KEYWORD, "de");
  analisar_expressao();
  consome(TOKEN_KEYWORD, "ate");
  analisar_expressao();
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "passo") == 0) {
    consome(TOKEN_KEYWORD, "passo");
    analisar_expressao();
  }
  consome(TOKEN_KEYWORD, "faca");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimpara");
}
void analisar_subrotinas(void) {
    while (lookahead.type == TOKEN_KEYWORD && 
      (strcmp(symtab[lookahead.attribute.table_index], "procedimento") == 0 || 
      strcmp(symtab[lookahead.attribute.table_index], "funcao") == 0)) {
        if (strcmp(symtab[lookahead.attribute.table_index], "procedimento") == 0) {
            analisar_declaracao_procedimento();
        } else {
            analisar_declaracao_funcao();
        }
    }
}
void analisar_declaracao_procedimento(void) {
  consome(TOKEN_KEYWORD, "procedimento");
  consome(TOKEN_ID, NULL);
  if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
    consome(TOKEN_DELIM, "(");
    analisar_lista_parametros();
    consome(TOKEN_DELIM, ")");
  }
  consome(TOKEN_KEYWORD, "inicio");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimprocedimento");
}
void analisar_lista_parametros(void) {
  consome(TOKEN_ID, NULL);
  consome(TOKEN_DELIM, ":");
  analisar_tipo();
  while (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
    consome(TOKEN_DELIM, ",");
    consome(TOKEN_ID, NULL);
    consome(TOKEN_DELIM, ":");
    analisar_tipo();
  }
}
void analisar_declaracao_funcao(void) {
  consome(TOKEN_KEYWORD, "funcao");
  consome(TOKEN_ID, NULL);
  consome(TOKEN_DELIM, "(");
  analisar_lista_parametros();
  consome(TOKEN_DELIM, ")");
  consome(TOKEN_DELIM, ":");
  analisar_tipo();
  consome(TOKEN_KEYWORD, "inicio");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimfuncao");
}

void erroSintatico(int linha, Token tokenIncorreto) {
  printf("ERRO SINTATICO na linha %d, token incorreto: ", linha);
  if (tokenIncorreto.type == TOKEN_NUM_INT) {
    printf("%d\n", tokenIncorreto.attribute.int_value);
  } else if (tokenIncorreto.type == TOKEN_NUM_FLOAT) {
    printf("%f\n", tokenIncorreto.attribute.float_value);
  } else if (tokenIncorreto.type == TOKEN_OP_REL) {
    printf("%s\n", nomeOpRel(tokenIncorreto.attribute.op_code));
  } else if (tokenIncorreto.type == TOKEN_EOF) {
    printf("EOF\n");
  } else if (tokenIncorreto.type == TOKEN_ID) {
    printf("IDENTIFICADOR %d\n", tokenIncorreto.attribute.table_index);
  } else {
    printf("%s\n", symtab[tokenIncorreto.attribute.table_index]);
  }
  exit(1);
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
    if (argc < 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 0;
    }

    FILE *arqFonte = fopen(argv[1], "rb");

    if (arqFonte == NULL) {
        printf("Erro ao abrir\n");
        return 0;
    }

    fseek(arqFonte, 0, SEEK_END);
    bufLen = ftell(arqFonte);
    fseek(arqFonte, 0, SEEK_SET);

    buffer = (char *) malloc((size_t) bufLen + 1);

    size_t lidos = fread(buffer, 1, (size_t) bufLen, arqFonte);
    buffer[lidos] = '\0';
    bufLen = (long) lidos;

    fclose(arqFonte);

    arqSaida = fopen("tokens_saida.txt", "w");

    if (arqSaida == NULL) {
        printf("Erro ao criar tokens_saida.txt\n");
        free(buffer);
        return 1;
    }

    // Inicializa o lookahead e inicia o parser
    nextToken();
    analisar_programa();

    // Depois de fimalgoritmo nao deve existir outro token
    if (lookahead.type != TOKEN_EOF) {
        erroSintatico(lookahead.line, lookahead);
    }

    // Liberacao dos recursos
    fclose(arqSaida);

    for (int i = 0; i < symCount; i++) {
        free(symtab[i]);
    }
    free(symtab);

    for (int i = 0; i < idCount; i++) {
        free(idtab[i]);
    }
    free(idtab);

    free(buffer);

    return 0;
}