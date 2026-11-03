/*
COMPILADORES - PROJETO - ETAPA 2: ANALISADOR LEXICO E SINTATICO
Linguagem: MiniVisualg
Mateus Ribeiro Cerqueira - 10443901
Pedro Henrique Carvalho Pereira - 10418861
*/

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_EOF, TOKEN_ID, TOKEN_NUM_INT, TOKEN_NUM_FLOAT,
    TOKEN_OP_REL, TOKEN_KEYWORD, TOKEN_STRING, TOKEN_OP_ARIT,
    TOKEN_ASSIGN, TOKEN_DELIM
} TokenNome;

typedef enum { OP_LT, OP_LE, OP_EQ, OP_GT, OP_GE, OP_NE } OpRelAtributo;

typedef struct {
    TokenNome type;
    int line;
    int symbol_index;
    union {
        int table_index;
        int int_value;
        double float_value;
        OpRelAtributo op_code;
    } attribute;
} Token;

typedef enum {
    SIM_IDENTIFICADOR, SIM_RESERVADA, SIM_INTEIRO, SIM_REAL,
    SIM_STRING, SIM_RELACIONAL, SIM_ARITMETICO, SIM_ATRIBUICAO,
    SIM_DELIMITADOR
} ClasseSimbolo;

typedef enum {
    TIPO_INDEFINIDO, TIPO_INTEIRO, TIPO_REAL, TIPO_CARACTERE,
    TIPO_LOGICO, TIPO_VETOR
} TipoSemantico;

typedef enum {
    PAPEL_INDEFINIDO, PAPEL_VARIAVEL, PAPEL_PARAMETRO,
    PAPEL_FUNCAO, PAPEL_PROCEDIMENTO
} PapelSimbolo;

typedef struct {
    char *lexema;
    ClasseSimbolo classe;
    TipoSemantico tipo;
    PapelSimbolo papel;
    int escopo;
    int endereco;
} Simbolo;

// tabela unica, com entradas numeradas a partir de 1
static Simbolo *tabelaSimbolos = NULL;
static int totalSimbolos = 0;
static int capacidadeSimbolos = 0;

static char *buffer = NULL;
static long tamanhoBuffer = 0;
static long posicao = 0;
static int linhaAtual = 1;
static FILE *arquivoTokens = NULL;
static Token lookahead;

static void salvarTabela(void);
static void erroLexico(int linha, const char *sequencia);
static void erroSintatico(int linha, Token t);
static void analisar_programa(void);
static void analisar_declaracoes(void);
static void analisar_tipo(void);
static void analisar_subrotinas(void);
static void analisar_bloco_comandos(void);
static void analisar_comando(void);
static void analisar_expressao(void);
static void analisar_lista_argumentos(void);
static void analisar_lista_parametros(void);

static void *alocar(size_t tamanho) {
    void *p = malloc(tamanho);
    if (!p) {
        fprintf(stderr, "Erro: memoria insuficiente\n");
        exit(1);
    }
    return p;
}

static char *copiarTrecho(long inicio, long fim) {
    size_t n = (size_t)(fim - inicio);
    char *texto = (char *)alocar(n + 1);
    memcpy(texto, buffer + inicio, n);
    texto[n] = '\0';
    return texto;
}

static int inserirSimbolo(const char *lexema, ClasseSimbolo classe) {
    for (int i = 0; i < totalSimbolos; i++) {
        if (tabelaSimbolos[i].classe == classe &&
            strcmp(tabelaSimbolos[i].lexema, lexema) == 0) {
            return i + 1;
        }
    }
    if (totalSimbolos == capacidadeSimbolos) {
        int nova = capacidadeSimbolos ? capacidadeSimbolos * 2 : 64;
        Simbolo *p = (Simbolo *)realloc(tabelaSimbolos, (size_t)nova * sizeof(Simbolo));
        if (!p) {
            fprintf(stderr, "Erro: memoria insuficiente\n");
            exit(1);
        }
        tabelaSimbolos = p;
        capacidadeSimbolos = nova;
    }
    Simbolo *s = &tabelaSimbolos[totalSimbolos];
    s->lexema = (char *)alocar(strlen(lexema) + 1);
    strcpy(s->lexema, lexema);
    s->classe = classe;
    s->tipo = TIPO_INDEFINIDO;
    s->papel = PAPEL_INDEFINIDO;
    s->escopo = -1;
    s->endereco = -1;
    return ++totalSimbolos;
}

static const char *textoSimbolo(int indice) {
    if (indice < 1 || indice > totalSimbolos) return "?";
    return tabelaSimbolos[indice - 1].lexema;
}

static const char *nomeClasse(ClasseSimbolo classe) {
    switch (classe) {
        case SIM_IDENTIFICADOR: return "IDENTIFICADOR";
        case SIM_RESERVADA: return "RESERVADA";
        case SIM_INTEIRO: return "NUM_INTEIRO";
        case SIM_REAL: return "NUM_REAL";
        case SIM_STRING: return "STRING";
        case SIM_RELACIONAL: return "OP_RELACIONAL";
        case SIM_ARITMETICO: return "OP_ARITMETICO";
        case SIM_ATRIBUICAO: return "ATRIBUICAO";
        case SIM_DELIMITADOR: return "DELIMITADOR";
    }
    return "?";
}

static const char *nomeTipo(TipoSemantico tipo) {
    switch (tipo) {
        case TIPO_INTEIRO: return "inteiro";
        case TIPO_REAL: return "real";
        case TIPO_CARACTERE: return "caractere";
        case TIPO_LOGICO: return "logico";
        case TIPO_VETOR: return "vetor";
        case TIPO_INDEFINIDO: return "-";
    }
    return "-";
}

static const char *nomePapel(PapelSimbolo papel) {
    switch (papel) {
        case PAPEL_VARIAVEL: return "variavel";
        case PAPEL_PARAMETRO: return "parametro";
        case PAPEL_FUNCAO: return "funcao";
        case PAPEL_PROCEDIMENTO: return "procedimento";
        case PAPEL_INDEFINIDO: return "-";
    }
    return "-";
}

static void escreverLexema(FILE *f, const char *lexema) {
    for (const unsigned char *p = (const unsigned char *)lexema; *p; p++) {
        if (*p == '\t') fputs("\\t", f);
        else if (*p == '\r') fputs("\\r", f);
        else if (*p == '\\') fputs("\\\\", f);
        else fputc(*p, f);
    }
}

static void salvarTabela(void) {
    FILE *f = fopen("tabela_simbolos.txt", "w");
    if (!f) {
        fprintf(stderr, "Erro ao criar tabela_simbolos.txt\n");
        return;
    }
    fprintf(f, "INDICE\tCLASSE\tLEXEMA\tESCOPO\tTIPO\tPAPEL\tENDERECO\n");
    for (int i = 0; i < totalSimbolos; i++) {
        const Simbolo *s = &tabelaSimbolos[i];
        char escopo[32] = "-", endereco[32] = "-";
        if (s->escopo >= 0) snprintf(escopo, sizeof(escopo), "%d", s->escopo);
        if (s->endereco >= 0) snprintf(endereco, sizeof(endereco), "%d", s->endereco);
        fprintf(f, "%d\t%s\t", i + 1, nomeClasse(s->classe));
        escreverLexema(f, s->lexema);
        fprintf(f, "\t%s\t%s\t%s\t%s\n", escopo,
                nomeTipo(s->tipo), nomePapel(s->papel), endereco);
    }
    fclose(f);
}

static void registrarErro(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vprintf(formato, args);
    va_end(args);
    if (arquivoTokens) {
        va_start(args, formato);
        vfprintf(arquivoTokens, formato, args);
        va_end(args);
        fflush(arquivoTokens);
    }
    salvarTabela();
    exit(1);
}

static const char *reservadas[] = {
    "algoritmo", "var", "inicio", "fimalgoritmo", "caractere", "inteiro",
    "real", "logico", "vetor", "de", "escreva", "escreval", "leia",
    "se", "entao", "senao", "fimse", "para", "ate", "passo", "faca",
    "fimpara", "enquanto", "fimenquanto", "procedimento", "fimprocedimento",
    "funcao", "fimfuncao", "retorne", "verdadeiro", "falso", "E", "OU",
    "mod", "MOD"
};

static int palavraReservada(const char *texto) {
    size_t n = sizeof(reservadas) / sizeof(reservadas[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcmp(reservadas[i], texto) == 0) return 1;
    }
    return 0;
}

static int fimArquivo(void) { return posicao >= tamanhoBuffer; }
static char atual(void) { return fimArquivo() ? '\0' : buffer[posicao]; }
static char seguinte(void) {
    return posicao + 1 >= tamanhoBuffer ? '\0' : buffer[posicao + 1];
}
static void avancar(void) {
    if (fimArquivo()) return;
    if (buffer[posicao] == '\n') linhaAtual++;
    posicao++;
}

static void pularEspacosEComentarios(void) {
    for (;;) {
        while (!fimArquivo() && isspace((unsigned char)atual())) avancar();
        if (atual() == '/' && seguinte() == '/') {
            while (!fimArquivo() && atual() != '\n') avancar();
        } else {
            break;
        }
    }
}

static Token criarToken(TokenNome tipo, ClasseSimbolo classe,
                        const char *lexema, int linha) {
    Token t = {0};
    t.type = tipo;
    t.line = linha;
    t.symbol_index = inserirSimbolo(lexema, classe);
    if (tipo == TOKEN_ID || tipo == TOKEN_STRING || tipo == TOKEN_KEYWORD ||
        tipo == TOKEN_OP_ARIT || tipo == TOKEN_ASSIGN || tipo == TOKEN_DELIM) {
        t.attribute.table_index = t.symbol_index;
    }
    return t;
}

Token obterToken(void) {
    pularEspacosEComentarios();
    int linha = linhaAtual;
    if (fimArquivo()) {
        Token fim = {0};
        fim.type = TOKEN_EOF;
        fim.line = linha;
        return fim;
    }

    char c = atual();
    long inicio = posicao;

    // o scanner le cada caractere, sem depender de espacos
    if (isalpha((unsigned char)c) || c == '_') {
        do { avancar(); }
        while (isalnum((unsigned char)atual()) || atual() == '_');
        char *lex = copiarTrecho(inicio, posicao);
        Token t;
        if (palavraReservada(lex)) {
            t = criarToken(TOKEN_KEYWORD, SIM_RESERVADA, lex, linha);
        } else {
            t = criarToken(TOKEN_ID, SIM_IDENTIFICADOR, lex, linha);
        }
        free(lex);
        return t;
    }

    if (isdigit((unsigned char)c)) {
        do { avancar(); } while (isdigit((unsigned char)atual()));
        int real = 0;
        if (atual() == '.' && isdigit((unsigned char)seguinte())) {
            real = 1;
            avancar();
            do { avancar(); } while (isdigit((unsigned char)atual()));
        }
        // numero junto com letra ou ponto solto nao segue a ER
        if (isalpha((unsigned char)atual()) || atual() == '_' ||
            (atual() == '.' && seguinte() != '.')) {
            while (isalnum((unsigned char)atual()) || atual() == '_' || atual() == '.') {
                avancar();
            }
            char *invalido = copiarTrecho(inicio, posicao);
            erroLexico(linha, invalido);
        }
        char *lex = copiarTrecho(inicio, posicao);
        Token t = criarToken(real ? TOKEN_NUM_FLOAT : TOKEN_NUM_INT,
                             real ? SIM_REAL : SIM_INTEIRO, lex, linha);
        errno = 0;
        if (real) {
            t.attribute.float_value = strtod(lex, NULL);
            if (errno == ERANGE) erroLexico(linha, lex);
        } else {
            long numero = strtol(lex, NULL, 10);
            if (errno == ERANGE || numero > INT_MAX) erroLexico(linha, lex);
            t.attribute.int_value = (int)numero;
        }
        free(lex);
        return t;
    }

    if (c == '"') {
        avancar();
        while (!fimArquivo() && atual() != '"' && atual() != '\n') avancar();
        if (atual() != '"') {
            char *invalido = copiarTrecho(inicio, posicao);
            erroLexico(linha, invalido);
        }
        avancar();
        char *lex = copiarTrecho(inicio, posicao);
        Token t = criarToken(TOKEN_STRING, SIM_STRING, lex, linha);
        free(lex);
        return t;
    }

    if (c == '<') {
        avancar();
        if (atual() == '-') {
            avancar();
            return criarToken(TOKEN_ASSIGN, SIM_ATRIBUICAO, "<-", linha);
        }
    }
    if (c == '<' || c == '>' || c == '=') {
        OpRelAtributo op;
        if (c == '<') {
            if (atual() == '=') { avancar(); op = OP_LE; }
            else if (atual() == '>') { avancar(); op = OP_NE; }
            else op = OP_LT;
        } else if (c == '>') {
            avancar();
            if (atual() == '=') { avancar(); op = OP_GE; }
            else op = OP_GT;
        } else {
            avancar();
            op = OP_EQ;
        }
        char *lex = copiarTrecho(inicio, posicao);
        Token t = criarToken(TOKEN_OP_REL, SIM_RELACIONAL, lex, linha);
        t.attribute.op_code = op;
        free(lex);
        return t;
    }

    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '\\') {
        avancar();
        char lex[] = {c, '\0'};
        return criarToken(TOKEN_OP_ARIT, SIM_ARITMETICO, lex, linha);
    }
    if (c == '.' && seguinte() == '.') {
        avancar(); avancar();
        return criarToken(TOKEN_DELIM, SIM_DELIMITADOR, "..", linha);
    }
    if (c == '(' || c == ')' || c == '[' || c == ']' || c == ',' || c == ':') {
        avancar();
        char lex[] = {c, '\0'};
        return criarToken(TOKEN_DELIM, SIM_DELIMITADOR, lex, linha);
    }

    char invalido[] = {c, '\0'};
    erroLexico(linha, invalido);
    return (Token){0};
}

static const char *nomeToken(TokenNome tipo) {
    switch (tipo) {
        case TOKEN_ID: return "IDENTIFICADOR";
        case TOKEN_NUM_INT: return "NUM_INTEIRO";
        case TOKEN_NUM_FLOAT: return "NUM_REAL";
        case TOKEN_OP_REL: return "OP_RELACIONAL";
        case TOKEN_STRING: return "STRING";
        case TOKEN_OP_ARIT: return "OP_ARITMETICO";
        case TOKEN_ASSIGN: return "ATRIBUICAO";
        case TOKEN_DELIM: return "DELIMITADOR";
        case TOKEN_KEYWORD: return "RESERVADA";
        case TOKEN_EOF: return "EOF";
    }
    return "?";
}

void infoToken(Token t) {
    // EOF so encerra a leitura interna
    if (t.type == TOKEN_EOF) return;
    char linha[1024];
    if (t.type == TOKEN_KEYWORD) {
        snprintf(linha, sizeof(linha), "%d# %s\n", t.line,
                 textoSimbolo(t.symbol_index));
    } else if (t.type == TOKEN_ID || t.type == TOKEN_STRING) {
        snprintf(linha, sizeof(linha), "%d# %s | %d\n", t.line,
                 nomeToken(t.type), t.attribute.table_index);
    } else if (t.type == TOKEN_NUM_INT) {
        snprintf(linha, sizeof(linha), "%d# %s | %d\n", t.line,
                 nomeToken(t.type), t.attribute.int_value);
    } else if (t.type == TOKEN_NUM_FLOAT) {
        snprintf(linha, sizeof(linha), "%d# %s | %g\n", t.line,
                 nomeToken(t.type), t.attribute.float_value);
    } else {
        snprintf(linha, sizeof(linha), "%d# %s | %s\n", t.line,
                 nomeToken(t.type), textoSimbolo(t.symbol_index));
    }
    fputs(linha, stdout);
    if (arquivoTokens) fputs(linha, arquivoTokens);
}

static void erroLexico(int linha, const char *sequencia) {
    registrarErro("ERRO LEXICO na linha %d: sequencia invalida \"%s\"\n",
                  linha, sequencia);
}

void nextToken(void) {
    lookahead = obterToken();
    infoToken(lookahead);
}

static const char *lexemaAtual(void) {
    return textoSimbolo(lookahead.symbol_index);
}

static int palavra(const char *s) {
    return lookahead.type == TOKEN_KEYWORD && strcmp(lexemaAtual(), s) == 0;
}

static int delimitador(const char *s) {
    return lookahead.type == TOKEN_DELIM && strcmp(lexemaAtual(), s) == 0;
}

static int aritmetico(const char *s) {
    return lookahead.type == TOKEN_OP_ARIT && strcmp(lexemaAtual(), s) == 0;
}

void consome(TokenNome tipo, const char *lexema) {
    if (lookahead.type != tipo ||
        (lexema && strcmp(lexemaAtual(), lexema) != 0)) {
        erroSintatico(lookahead.line, lookahead);
    }
    nextToken();
}

static void analisar_tipo(void) {
    if (palavra("caractere")) consome(TOKEN_KEYWORD, "caractere");
    else if (palavra("inteiro")) consome(TOKEN_KEYWORD, "inteiro");
    else if (palavra("real")) consome(TOKEN_KEYWORD, "real");
    else if (palavra("logico")) consome(TOKEN_KEYWORD, "logico");
    else if (palavra("vetor")) {
        consome(TOKEN_KEYWORD, "vetor");
        consome(TOKEN_DELIM, "[");
        consome(TOKEN_NUM_INT, NULL);
        consome(TOKEN_DELIM, "..");
        consome(TOKEN_NUM_INT, NULL);
        consome(TOKEN_DELIM, "]");
        consome(TOKEN_KEYWORD, "de");
        analisar_tipo();
    } else erroSintatico(lookahead.line, lookahead);
}

static void analisar_declaracoes(void) {
    while (lookahead.type == TOKEN_ID) {
        consome(TOKEN_ID, NULL);
        while (delimitador(",")) {
            consome(TOKEN_DELIM, ",");
            consome(TOKEN_ID, NULL);
        }
        consome(TOKEN_DELIM, ":");
        analisar_tipo();
    }
}

static void analisar_lista_parametros(void) {
    // a lista pode ser vazia na declaracao de uma funcao
    if (lookahead.type != TOKEN_ID) return;
    consome(TOKEN_ID, NULL);
    consome(TOKEN_DELIM, ":");
    analisar_tipo();
    while (delimitador(",")) {
        consome(TOKEN_DELIM, ",");
        consome(TOKEN_ID, NULL);
        consome(TOKEN_DELIM, ":");
        analisar_tipo();
    }
}

static void analisar_declaracao_procedimento(void) {
    consome(TOKEN_KEYWORD, "procedimento");
    consome(TOKEN_ID, NULL);
    if (delimitador("(")) {
        consome(TOKEN_DELIM, "(");
        if (lookahead.type != TOKEN_ID) erroSintatico(lookahead.line, lookahead);
        analisar_lista_parametros();
        consome(TOKEN_DELIM, ")");
    }
    if (palavra("var")) {
        consome(TOKEN_KEYWORD, "var");
        analisar_declaracoes();
    }
    consome(TOKEN_KEYWORD, "inicio");
    analisar_bloco_comandos();
    consome(TOKEN_KEYWORD, "fimprocedimento");
}

static void analisar_declaracao_funcao(void) {
    consome(TOKEN_KEYWORD, "funcao");
    consome(TOKEN_ID, NULL);
    consome(TOKEN_DELIM, "(");
    analisar_lista_parametros();
    consome(TOKEN_DELIM, ")");
    consome(TOKEN_DELIM, ":");
    analisar_tipo();
    if (palavra("var")) {
        consome(TOKEN_KEYWORD, "var");
        analisar_declaracoes();
    }
    consome(TOKEN_KEYWORD, "inicio");
    analisar_bloco_comandos();
    consome(TOKEN_KEYWORD, "fimfuncao");
}

static void analisar_subrotinas(void) {
    while (palavra("procedimento") || palavra("funcao")) {
        if (palavra("procedimento")) analisar_declaracao_procedimento();
        else analisar_declaracao_funcao();
    }
}

static void analisar_programa(void) {
    consome(TOKEN_KEYWORD, "algoritmo");
    consome(TOKEN_STRING, NULL);
    int temVariaveis = 0;
    if (palavra("var")) {
        consome(TOKEN_KEYWORD, "var");
        analisar_declaracoes();
        temVariaveis = 1;
    }
    analisar_subrotinas();
    if (!temVariaveis && palavra("var")) {
        consome(TOKEN_KEYWORD, "var");
        analisar_declaracoes();
    }
    consome(TOKEN_KEYWORD, "inicio");
    analisar_bloco_comandos();
    consome(TOKEN_KEYWORD, "fimalgoritmo");
}

static int fimBloco(void) {
    return palavra("fimalgoritmo") || palavra("senao") || palavra("fimse") ||
           palavra("fimpara") || palavra("fimenquanto") ||
           palavra("fimprocedimento") || palavra("fimfuncao");
}

static void analisar_bloco_comandos(void) {
    while (!fimBloco()) analisar_comando();
}

static void analisar_fator(void) {
    if (aritmetico("+") || aritmetico("-")) {
        consome(TOKEN_OP_ARIT, lexemaAtual());
        analisar_fator();
    } else if (lookahead.type == TOKEN_NUM_INT ||
               lookahead.type == TOKEN_NUM_FLOAT ||
               lookahead.type == TOKEN_STRING) {
        consome(lookahead.type, NULL);
    } else if (palavra("verdadeiro") || palavra("falso")) {
        consome(TOKEN_KEYWORD, lexemaAtual());
    } else if (lookahead.type == TOKEN_ID) {
        consome(TOKEN_ID, NULL);
        if (delimitador("(")) {
            consome(TOKEN_DELIM, "(");
            analisar_lista_argumentos();
            consome(TOKEN_DELIM, ")");
        } else if (delimitador("[")) {
            consome(TOKEN_DELIM, "[");
            analisar_expressao();
            consome(TOKEN_DELIM, "]");
        }
    } else if (delimitador("(")) {
        consome(TOKEN_DELIM, "(");
        analisar_expressao();
        consome(TOKEN_DELIM, ")");
    } else erroSintatico(lookahead.line, lookahead);
}

static void analisar_termo(void) {
    analisar_fator();
    while (aritmetico("*") || aritmetico("/") || aritmetico("\\") ||
           palavra("mod") || palavra("MOD")) {
        consome(lookahead.type, lexemaAtual());
        analisar_fator();
    }
}

static void analisar_expr_aritmetica(void) {
    analisar_termo();
    while (aritmetico("+") || aritmetico("-")) {
        consome(TOKEN_OP_ARIT, lexemaAtual());
        analisar_termo();
    }
}

static void analisar_expr_relacional(void) {
    analisar_expr_aritmetica();
    while (lookahead.type == TOKEN_OP_REL) {
        consome(TOKEN_OP_REL, NULL);
        analisar_expr_aritmetica();
    }
}

static void analisar_expr_logica(void) {
    analisar_expr_relacional();
    while (palavra("E") || palavra("OU")) {
        consome(TOKEN_KEYWORD, lexemaAtual());
        analisar_expr_relacional();
    }
}

static void analisar_expressao(void) { analisar_expr_logica(); }

static void analisar_lista_argumentos(void) {
    if (delimitador(")")) return;
    analisar_expressao();
    while (delimitador(",")) {
        consome(TOKEN_DELIM, ",");
        analisar_expressao();
    }
}

static void analisar_escrita(void) {
    consome(TOKEN_KEYWORD, lexemaAtual());
    consome(TOKEN_DELIM, "(");
    analisar_expressao();
    while (delimitador(",")) {
        consome(TOKEN_DELIM, ",");
        analisar_expressao();
    }
    consome(TOKEN_DELIM, ")");
}

static void analisar_leitura(void) {
    consome(TOKEN_KEYWORD, "leia");
    consome(TOKEN_DELIM, "(");
    consome(TOKEN_ID, NULL);
    if (delimitador("[")) {
        consome(TOKEN_DELIM, "[");
        analisar_expressao();
        consome(TOKEN_DELIM, "]");
    }
    consome(TOKEN_DELIM, ")");
}

static void analisar_condicional(void) {
    consome(TOKEN_KEYWORD, "se");
    consome(TOKEN_DELIM, "(");
    analisar_expressao();
    consome(TOKEN_DELIM, ")");
    consome(TOKEN_KEYWORD, "entao");
    analisar_bloco_comandos();
    if (palavra("senao")) {
        consome(TOKEN_KEYWORD, "senao");
        analisar_bloco_comandos();
    }
    consome(TOKEN_KEYWORD, "fimse");
}

static void analisar_repeticao_para(void) {
    consome(TOKEN_KEYWORD, "para");
    consome(TOKEN_ID, NULL);
    consome(TOKEN_KEYWORD, "de");
    analisar_expressao();
    consome(TOKEN_KEYWORD, "ate");
    analisar_expressao();
    if (palavra("passo")) {
        consome(TOKEN_KEYWORD, "passo");
        analisar_expressao();
    }
    consome(TOKEN_KEYWORD, "faca");
    analisar_bloco_comandos();
    consome(TOKEN_KEYWORD, "fimpara");
}

static void analisar_repeticao_enquanto(void) {
    consome(TOKEN_KEYWORD, "enquanto");
    consome(TOKEN_DELIM, "(");
    analisar_expressao();
    consome(TOKEN_DELIM, ")");
    consome(TOKEN_KEYWORD, "faca");
    analisar_bloco_comandos();
    consome(TOKEN_KEYWORD, "fimenquanto");
}

static void analisar_comando(void) {
    if (palavra("escreva") || palavra("escreval")) analisar_escrita();
    else if (palavra("leia")) analisar_leitura();
    else if (palavra("se")) analisar_condicional();
    else if (palavra("para")) analisar_repeticao_para();
    else if (palavra("enquanto")) analisar_repeticao_enquanto();
    else if (palavra("retorne")) {
        consome(TOKEN_KEYWORD, "retorne");
        analisar_expressao();
    } else if (lookahead.type == TOKEN_ID) {
        consome(TOKEN_ID, NULL);
        if (delimitador("[")) {
            consome(TOKEN_DELIM, "[");
            analisar_expressao();
            consome(TOKEN_DELIM, "]");
            consome(TOKEN_ASSIGN, NULL);
            analisar_expressao();
        } else if (lookahead.type == TOKEN_ASSIGN) {
            consome(TOKEN_ASSIGN, NULL);
            analisar_expressao();
        } else if (delimitador("(")) {
            consome(TOKEN_DELIM, "(");
            analisar_lista_argumentos();
            consome(TOKEN_DELIM, ")");
        }
    } else erroSintatico(lookahead.line, lookahead);
}

static void erroSintatico(int linha, Token t) {
    if (t.type == TOKEN_EOF) {
        registrarErro("ERRO SINTATICO na linha %d, token incorreto: EOF\n", linha);
    } else {
        registrarErro("ERRO SINTATICO na linha %d, token incorreto: %s\n",
                      linha, textoSimbolo(t.symbol_index));
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }
    FILE *fonte = fopen(argv[1], "rb");
    if (!fonte) {
        fprintf(stderr, "Erro ao abrir %s\n", argv[1]);
        return 1;
    }
    if (fseek(fonte, 0, SEEK_END) != 0 || (tamanhoBuffer = ftell(fonte)) < 0 ||
        fseek(fonte, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao ler arquivo fonte\n");
        fclose(fonte);
        return 1;
    }
    buffer = (char *)alocar((size_t)tamanhoBuffer + 1);
    size_t lidos = fread(buffer, 1, (size_t)tamanhoBuffer, fonte);
    if (ferror(fonte)) {
        fprintf(stderr, "Erro ao ler arquivo fonte\n");
        fclose(fonte);
        free(buffer);
        return 1;
    }
    fclose(fonte);
    tamanhoBuffer = (long)lidos;
    buffer[tamanhoBuffer] = '\0';

    arquivoTokens = fopen("tokens_saida.txt", "w");
    if (!arquivoTokens) {
        fprintf(stderr, "Erro ao criar tokens_saida.txt\n");
        free(buffer);
        return 1;
    }
    nextToken();
    analisar_programa();
    if (lookahead.type != TOKEN_EOF) erroSintatico(lookahead.line, lookahead);
    salvarTabela();
    fclose(arquivoTokens);
    for (int i = 0; i < totalSimbolos; i++) free(tabelaSimbolos[i].lexema);
    free(tabelaSimbolos);
    free(buffer);
    return 0;
}
