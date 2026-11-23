/*
COMPILADORES - PROJETO - Fase 1: Analise Lexica e Analise Sintatica
Fase 2: Analise Semantica e Geracao de Codigo Intermediario (MEPA)
Linguagem: MiniVisualg (Visualg Simplificado)

Integrantes do grupo:
  - Mateus Ribeiro Cerqueira - 10443901
  - Pedro Henrique Carvalho Pereira - 10418861

Compilacao:
  gcc -g -Og -Wall compilador.c -o compilador

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
    // (feito) TODO: avaliar necessidade de tokens adicionais para cobrir toda a
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
    OP_EQ, // == (Equal) -- (feito) TODO: confirmar simbolo usado no MiniVisualg (Anexo I usa "=")
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

// (feito) TODO: definir estrutura da Tabela de Simbolos (nao detalhada explicitamente
// na Figura 2, mas necessaria pois Token.attribute.table_index referencia
// indices dela). Deve armazenar pelo menos o lexema (nome) do identificador.

/*
SECAO 1.1: MINITABELA DE SIMBOLOS - FASE 2

TNo em lista encadeada. A tabela acima, da Fase 1,
continua sendo usada pelo lex. 
As duas tabelas tem funcoes diferentes.
*/
typedef struct _TNo {
    char ID[16];
    int endereco;
    char tipo[7];
    struct _TNo *prox;
} TNo;

/*
TODO (FASE 2): Insercao na minitabela.
  1. Criar um TNo para cada variavel declarada e adicionar ao fim da lista.
  2. Guardar a ordem da declaracao em endereco: 0, 1, 2, ...
  3. Verificar nome repetido e nome maior que 15 caracteres.
  4. Usar nomes curtos em tipo (int, real, char, log, v_int, etc.),
     pois o campo tem apenas 7 caracteres.
  5. Nao confundir esse endereco com a posicao na pilha da MEPA.
*/

/* Tabela de Simbolos: guarda o texto (lexema) de identificadores, strings,
   palavras reservadas e operadores/delimitadores, referenciados pelos
   tokens atraves de attribute.table_index. */
#define SYM_CAP_INICIAL 64
static char **symtab = NULL;
static int symCount = 0;
static int symCap = 0;

static int addSymbol(const char *s) {
  if (symCount >= symCap) {
    if (symCap == 0) {
      symCap = SYM_CAP_INICIAL;
    } else {
      symCap = symCap * 2;
    }
    symtab = (char **) realloc(symtab, (size_t) symCap * sizeof(char *));
  }
  symtab[symCount] = (char *) malloc(strlen(s) + 1);
  strcpy(symtab[symCount], s);
  symCount++;
  return symCount - 1;
}

/* Tabela de id apenas para reconhecer repeticoes
   O token guarda somente o NUMERO do identificador (1, 2, 3...). */
// correcao da profa sobre o padrao ID, o atributo agora e a posicao
// correspondente na tabela de simbolos (symtab), nao a ordem da idtab
static char **idtab = NULL;
static int idCount = 0;
static int idCap = 0;

// Retorna o numero (1-based) do identificador, cadastrando se for novo
// correcao da profa retorna o indice real da symtab
static int findOrAddIdentifier(const char *s) {
  for (int i = 0; i < idCount; i++) {
    if (strcmp(idtab[i], s) == 0) {
      for (int j = 0; j < symCount; j++) {
        if (symtab[j] == idtab[i]) return j;
      }
    }
  }
  if (idCount >= idCap) {
    if (idCap == 0) {
      idCap = SYM_CAP_INICIAL;
    } else {
      idCap = idCap * 2;
    }
    idtab = (char **) realloc(idtab, (size_t) idCap * sizeof(char *));
  }
  // correcao da prof ID cadastrado na tabela principal uma unica vez
  int indice = addSymbol(s);
  idtab[idCount] = symtab[indice];
  idCount++;
  return indice;
}

//SECAO 2: PROTOTIPOS

// ---- Analisador Lexico (Scanner) ----
// (feitos) TODO: declarar variaveis globais/estado do scanner:
//   - char *buffer          (conteudo do arquivo fonte carregado em memoria)
//   - posicao atual de leitura no buffer
//   - linha atual (para reporte de erros)
//   - arquivo de saida dos tokens

static char *buffer = NULL;   /* conteudo do arquivo fonte carregado em memoria */
static long bufLen = 0;
static long bufPos = 0;
static int linhaAtual = 1; // famoso linhaAtual kkk
static FILE *arqSaida = NULL; /* arquivo de saida dos tokens */


Token obterToken(void);
void infoToken(Token t);

// ---- Analisador Sintatico (Parser) ----
// (feito) TODO: declarar variavel global do parser:
//   - Token lookahead (token atual sob analise, conforme Figura 1: "char lookahead")
static Token lookahead;
// infos simples da Fase 2, ainda nao integradas ao lex
static TNo *minitabela = NULL;
static int contRotulos = 0;

/* ---- (fase 2) dados auxiliares ----
   a TNo nao pode mudar (enunciado), entao tudo que a minitabela nao guarda
   (tipo base, vetor, atribuida/usada, rotinas...) fica nessas estruturas aqui do lado. 
   solução proposta pra adequar mepa e GLC*/

// codigos de tipo: é isso que os analisar_* de expressao devolvem pro chamador
#define TIPO_NENHUM 0 // procedimento (nao devolve valor)
#define TIPO_INT 1
#define TIPO_REAL 2
#define TIPO_CHAR 3
#define TIPO_LOG 4

#define MAX_VARS 512
#define MAX_ROTINAS 64
#define MAX_PARAMS 16
#define MAX_COD 8192

// uma InfoVar pra cada no da minitabela (mesma ordem de declaracao)
typedef struct {
  TNo *no;
  int tipo; // tipo base: TIPO_INT, TIPO_REAL...
  int ehVetor;
  int lo; // limites do vetor
  int hi;
  int ehParam;
  int atribuida; // recebeu valor (<- ou leia)
  int usada; // foi lida em expressao, escreva, condicao, indice...
  int linhaAtrib; // linha da 1a atribuicao (pro erro de variavel nao usada)
} InfoVar;

static InfoVar infoVar[MAX_VARS];
static int nVars = 0;
static TNo *fimMinitabela = NULL; // ultimo no da lista (insercao no fim)
static int proxEndereco = 0; // proximo endereco livre (vetor ocupa varios)





// tabela auxiliar de rotinas (procedimentos/ funcoes)
typedef struct {
  char nome[64];
  int ehFuncao;
  int tipoRet;
  int nParam;
  int tipoParam[MAX_PARAMS];
  char nomeParam[MAX_PARAMS][16];
  int rotulo; // rotulo MEPA da entrada da rotina
  int rotuloFim; // rotulo do RTPR (retorne pula pra ca)
} InfoRotina;


static InfoRotina rotinas[MAX_ROTINAS];
static int nRotinas = 0;
static int rotinaAtual = -1; // -1 = programa principal

static int linhaUltimoId = 1; // linha do ultimo identificador consumido (pros erros)
static int contaTokens = 0; // qtd de tokens ja lidos (usado pra detectar indice constante)
static int rotuloMain = 0; // rotulo do bloco principal

// vars do vetor de tipo (analisar_tipo devolve o tipo, e os limites ficam aqui)
static int tipoEhVetor = 0;
static int vetLo = 0;
static int vetHi = 0;







// codigo MEPA fica num buffer, so vai pro arquivo no fim se nao teve erro
// isso corresponde ao comentário da prof sobre não passar pra geração de código (acho)
static char codigo[MAX_COD][96];
static int nCod = 0;

//gambiarra com o MEPA dado :D
static int semMepa = 0; // 1 = programa usa algo que a mepa nao roda (real, caractere...)
static char motivoSemMepa[120];
static int usaTmpVet = 0; // usou os temporarios $tv1/$tv2 (acesso a vetor)
static int usaTmpMod = 0; // usou os temporarios $tma/$tmb (mod)

/*
TODO (FASE 2): Informacoes complementares.
  1. Guardar se cada variavel foi atribuida e se foi usada em leitura.
  2. Guardar nivel e deslocamento para acesso a variaveis na MEPA.
  3. Guardar tamanho e limites de vetores, quando existirem.
  4. Guardar nome, parametros e retorno das funcoes e procedimentos.
  5. Fazer isso sem alterar a estrutura obrigatoria de TNo.
*/

void nextToken(void);

// (feito) TODO: declarar protótipos das funcoes de descida recursiva, uma para cada
// nao-terminal definido na gramatica da Etapa 1 (etapa1_gramatica.txt).
// Exemplos esperados a partir do Anexo I (ajustar nomes conforme a GLC final):

// ---- Analisador Sintatico (Parser) ----

void consome(TokenNome tipo, const char *lexema);
void analisar_programa(void);
void analisar_declaracoes(void);
int analisar_tipo(void);
void analisar_bloco_comandos(void);
int eh_fim_bloco(void);
void analisar_comando(void);
void analisar_leitura(void);
void analisar_escrita(void);
void analisar_condicional(void);
void analisar_repeticao_para(void);
void analisar_repeticao_enquanto(void);
void analisar_lista_argumentos(int idRot);
int analisar_expressao(void);
int analisar_expr_logica(void);
int analisar_expr_e(void); // (fase 2) novo nivel: E tem prioridade sobre OU
int analisar_expr_relacional(void);
int analisar_expr_aritmetica(void);
int analisar_termo(void);
int analisar_fator(void);
void analisar_subrotinas(void);
void analisar_declaracao_procedimento(void);
void analisar_declaracao_funcao(void);
void analisar_lista_parametros(void);

//====================================
void erroSintatico(int linha, Token tokenIncorreto);
void erroLexico(int linha, const char *sequencia);
void erroSemantico(int linha, const char *motivo, const char *nome);
//====================================

static int ehPalavraReservada(const char *lexema); // definida la na secao do scanner




/*
TODO (FASE 2): proximo_rotulo().
  Devolver rotulos consecutivos para desvios e rotinas (1, 2, 3...).
  Guardar o retorno em uma variavel local antes de analisar blocos
  recursivamente, para nao trocar os destinos dos desvios.
*/
int proximo_rotulo(void) {
  contRotulos++;
  return contRotulos;
}

/*
TODO (FASE 2): busca_tabela_simbolos().
  Receber o nome do identificador, percorrer a lista TNo e devolver
  o endereco da declaracao. Um nome ausente gera erro semantico.
  O parser ainda precisa chamar esta funcao nos lugares apropriados.
*/
int busca_tabela_simbolos(const char *atributo_ID) {
  TNo *atual = minitabela;

  while (atual != NULL) {
    if (strcmp(atual->ID, atributo_ID) == 0) {
      return atual->endereco;
    }
    atual = atual->prox;
  }

  // daqui o fluxo vai pra erroSemantico() e o programa termina
  erroSemantico(linhaUltimoId, "identificador nao declarado", atributo_ID);
  return -1;
}

/*
TODO (FASE 2): erro semantico.
  1. Informar a linha e o motivo do erro, como pede o enunciado.
  2. Encerrar a compilacao quando alguma regra semantica falhar.
  3. Manter a saida de tokens da Fase 1 sem mensagens extras em
     programas validos. Nao salvar codigo MEPA se ocorrer erro.
*/

// erro semantico: mostra linha + motivo (+ nome entre aspas) e encerra. não passa para mepa_cod.txt
void erroSemantico(int linha, const char *motivo, const char *nome) {
  printf("ERRO SEMANTICO na linha %d: %s", linha, motivo);
  if (nome[0] != '\0') printf(" '%s'", nome);
  printf("\n");
  exit(1);
}


/*
SECAO 2.1: AUXILIARES DA FASE 2
tipos, minitabela, tabela de rotinas e emissao de codigo MEPA.
tudo simples, sem recursao, quem chama sao os analisar_* mais abaixo.
*/

// ---- tipos ----
static const char *nome_tipo(int t) {
  if (t == TIPO_INT) return "inteiro";
  if (t == TIPO_REAL) return "real";
  if (t == TIPO_CHAR) return "caractere";
  if (t == TIPO_LOG) return "logico";
  return "nenhum";
}

// nome curto que vai no campo tipo[7] do TNo (sem criatividade pra nome rs)
static const char *tipo_curto(int t) {
  if (t == TIPO_INT) return "int";
  if (t == TIPO_REAL) return "real";
  if (t == TIPO_CHAR) return "char";
  return "log";
}

static int tipo_numerico(int t) {
  return t == TIPO_INT || t == TIPO_REAL;
}

// destino <- origem: igual pode, real <- inteiro pode, o resto nao
static int tipo_atribuivel(int destino, int origem) {
  if (destino == origem) return 1;
  if (destino == TIPO_REAL && origem == TIPO_INT) return 1;
  return 0;
}

// resultado de + - * : real se algum for real, senao inteiro
static int tipo_aritmetico(int a, int b) {
  if (a == TIPO_REAL || b == TIPO_REAL) return TIPO_REAL;
  return TIPO_INT;
}

static void erro_tipos(int linha, const char *onde, int t1, int t2) {
  char msg[200];
  snprintf(msg, sizeof(msg), "tipos incompativeis em %s (%s e %s)", onde, nome_tipo(t1), nome_tipo(t2));
  erroSemantico(linha, msg, "");
}

// marca que a mepa nao roda esse programa (a analise semantica continua normal)
static void marca_sem_mepa(const char *motivo) {
  if (semMepa == 0) {
    semMepa = 1;
    snprintf(motivoSemMepa, sizeof(motivoSemMepa), "%s", motivo);
  }
}




// ---- minitabela (lista encadeada TNo) ----
static TNo *acha_no(const char *nome) {
  TNo *atual = minitabela;
  while (atual != NULL) {
    if (strcmp(atual->ID, nome) == 0) return atual;
    atual = atual->prox;
  }
  return NULL;
}

// chama a busca_tabela_simbolos() do enunciado (ela da o erro se nao existir) e devolve o no
static TNo *no_declarado(const char *nome) {
  busca_tabela_simbolos(nome);
  return acha_no(nome);
}

static InfoVar *busca_info(TNo *no) {
  int i;
  for (i = 0; i < nVars; i++) {
    if (infoVar[i].no == no) return &infoVar[i];
  }
  return &infoVar[0]; // nao deve acontecer, todo no tem info
}

static int busca_rotina(const char *nome) {
  int i;
  for (i = 0; i < nRotinas; i++) {
    if (strcmp(rotinas[i].nome, nome) == 0) return i;
  }
  return -1;
}





// insere no fim da lista. endereco = ordem de declaracao (vetor ocupa tam posicoes)
static void insere_variavel(const char *nome, int linha, int tipo, int ehVetor, int lo, int hi, int ehParam) {
  TNo *novo;
  int tam = 1;


  // contenção erros sem
  if (strlen(nome) > 15) erroSemantico(linha, "nome de variavel com mais de 15 caracteres", nome);
  if (ehPalavraReservada(nome)) erroSemantico(linha, "palavra reservada nao pode ser nome de variavel", nome);
  if (acha_no(nome) != NULL) erroSemantico(linha, "identificador ja declarado", nome);
  if (busca_rotina(nome) >= 0) erroSemantico(linha, "nome ja usado por uma rotina", nome);
  if (nVars >= MAX_VARS) erroSemantico(linha, "variaveis demais no programa", nome);

  if (ehVetor) tam = hi - lo + 1;

  novo = (TNo *) malloc(sizeof(TNo));
  strcpy(novo->ID, nome);
  // na TNo o endereco e a ordem de declaracao; a posicao mepa continua no proxEndereco
  novo->endereco = nVars;
  // tipo[7]: "int", "real", "char", "log" ou com prefixo v_ pra vetor (v_real = 6 + \0)
  novo->tipo[0] = '\0';
  if (ehVetor) strcpy(novo->tipo, "v_");
  strcat(novo->tipo, tipo_curto(tipo));
  novo->prox = NULL;
  proxEndereco = proxEndereco + tam;

  if (minitabela == NULL) minitabela = novo;
  else fimMinitabela->prox = novo;
  fimMinitabela = novo;

  infoVar[nVars].no = novo;
  infoVar[nVars].tipo = tipo;
  infoVar[nVars].ehVetor = ehVetor;
  infoVar[nVars].lo = lo;
  infoVar[nVars].hi = hi;
  infoVar[nVars].ehParam = ehParam;
  infoVar[nVars].atribuida = 0;
  infoVar[nVars].usada = 0;
  infoVar[nVars].linhaAtrib = linha;
  nVars++;
}

static void marca_atribuida(InfoVar *info, int linha) {
  if (info->atribuida == 0) {
    info->atribuida = 1;
    info->linhaAtrib = linha;
  }
}

// variavel que recebeu valor e nunca foi lida = erro (parametro fica de fora)
static void confere_variaveis_nao_usadas(void) {
  int i;
  for (i = 0; i < nVars; i++) {
    if (infoVar[i].ehParam == 0 && infoVar[i].atribuida == 1 && infoVar[i].usada == 0) {
      erroSemantico(infoVar[i].linhaAtrib, "variavel recebeu valor mas nunca foi usada", infoVar[i].no->ID);
    }
  }
}

// ---- rotinas ----
// cadastra antes de ler o corpo, assim a recursao direta funciona
static int registra_rotina(const char *nome, int linha, int ehFuncao) {
  int id;
  int rot;
  int rotFim;
  if (strlen(nome) > 60) erroSemantico(linha, "nome de rotina grande demais", nome);
  if (busca_rotina(nome) >= 0) erroSemantico(linha, "rotina ja declarada", nome);
  if (acha_no(nome) != NULL) erroSemantico(linha, "nome ja usado por uma variavel", nome);
  if (nRotinas >= MAX_ROTINAS) erroSemantico(linha, "rotinas demais no programa", nome);

  // guarda nas locais antes de usar (ordem das chamadas do proximo_rotulo importa)
  rot = proximo_rotulo();
  rotFim = proximo_rotulo();

  id = nRotinas;
  strcpy(rotinas[id].nome, nome);
  rotinas[id].ehFuncao = ehFuncao;
  rotinas[id].tipoRet = TIPO_NENHUM;
  rotinas[id].nParam = 0;
  rotinas[id].rotulo = rot;
  rotinas[id].rotuloFim = rotFim;
  nRotinas++;
  return id;
}

// ---- leitura de nomes ----
// consome um ID e guarda o lexema em nome (buffer de 256) e a linha em linhaUltimoId
static void consome_id(char *nome) {
  if (lookahead.type != TOKEN_ID) erroSintatico(lookahead.line, lookahead);
  strcpy(nome, symtab[lookahead.attribute.table_index]);
  linhaUltimoId = lookahead.line;
  consome(TOKEN_ID, NULL);
}
// nome numa declaracao (var ou parametro): se vier palavra reservada e erro semantico
static void le_nome_declaracao(char *nome, int *linha) {
  if (lookahead.type == TOKEN_KEYWORD) {
    erroSemantico(lookahead.line, "palavra reservada nao pode ser nome de variavel", symtab[lookahead.attribute.table_index]);
  }
  consome_id(nome);
  *linha = linhaUltimoId;
}




// ---- emissao de codigo MEPA (vai pro buffer 'codigo') ----
static void emite(const char *instr) {
  if (nCod >= MAX_COD) {
    marca_sem_mepa("programa grande demais pro buffer de codigo");
    return;
  }
  snprintf(codigo[nCod], sizeof(codigo[nCod]), "%s", instr);
  nCod++;
}

static void emite_num(const char *instr, int n) {
  char buf[96];
  snprintf(buf, sizeof(buf), "%s %d", instr, n);
  emite(buf);
}

static void emite_nome(const char *instr, const char *nome) {
  char buf[96];
  snprintf(buf, sizeof(buf), "%s %s", instr, nome);
  emite(buf);
}

static void emite_rot(const char *instr, int rot) {
  char buf[96];
  snprintf(buf, sizeof(buf), "%s L%d", instr, rot);
  emite(buf);
}

// rotulo sempre vai numa linha NADA ("L3: NADA"), como nos padroes de desvio
static void emite_rotulo(int rot) {
  char buf[96];
  snprintf(buf, sizeof(buf), "L%d: NADA", rot);
  emite(buf);
}

// retorno da funcao fica num slot proprio: "$ret_nome" (o $ nao existe em nome de var do usuario)
static void emite_ret(const char *instr, int idRot) {
  char buf[96];
  snprintf(buf, sizeof(buf), "%s $ret_%s", instr, rotinas[idRot].nome);
  emite(buf);
}

// a MEPA nao tem MOD direto: a mod b = a - (a / b) * b. os dois operandos ja estao na pilha,
// entao guarda nos temporarios $tma/$tmb pra poder usar cada um duas vezes
static void gera_mod(void) {
  usaTmpMod = 1;
  emite_nome("ARMZ", "$tmb");
  emite_nome("ARMZ", "$tma");
  emite_nome("CRVL", "$tma");
  emite_nome("CRVL", "$tma");
  emite_nome("CRVL", "$tmb");
  emite("DIVI");
  emite_nome("CRVL", "$tmb");
  emite("MULT");
  emite("SUBT");
}

// chamada de rotina: (args) opcionais + passagem por valor + CHPR
// os argumentos vao pra pilha em ordem e depois sao guardados (de tras pra frente)
// nas posicoes dos parametros. param = variavel normal da minitabela (alocacao estatica).
static void chama_rotina(int idRot, int linha) {
  int i;
  char buf[96];
  if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
    // fix do procedimento sem params (correcao do anexo 1)
    if (rotinas[idRot].ehFuncao == 0 && rotinas[idRot].nParam == 0) erroSintatico(lookahead.line, lookahead);
    consome(TOKEN_DELIM, "(");
    analisar_lista_argumentos(idRot); // daqui o fluxo vai pra analisar_lista_argumentos() e volta pra ca
    consome(TOKEN_DELIM, ")");
  } else if (rotinas[idRot].nParam != 0) {
    erroSemantico(linha, "numero de argumentos incompativel na chamada de", rotinas[idRot].nome);
  }
  for (i = rotinas[idRot].nParam - 1; i >= 0; i--) {
    emite_nome("ARMZ", rotinas[idRot].nomeParam[i]);
  }
  snprintf(buf, sizeof(buf), "CHPR L%d, 0", rotinas[idRot].rotulo);
  emite(buf);
}


// acesso a vetor: le [ indice ], confere tipo/limite e deixa o ENDERECO do elemento
// no topo da pilha: indice - lo + base. quem chamou decide o que fazer com ele
// (CRVI / ARMI precisam do endereco guardado numa variavel, ver $tv1/$tv2)
static void gera_endereco_vetor(InfoVar *info, const char *nome, int linha) {
  int tipoIdx;
  int ehLit;
  int valLit = 0;
  int tokensAntes;
  consome(TOKEN_DELIM, "[");
  ehLit = (lookahead.type == TOKEN_NUM_INT);
  if (ehLit) valLit = lookahead.attribute.int_value;
  tokensAntes = contaTokens;
  tipoIdx = analisar_expressao();
  // o MEPA dado so usa inteiro, então essa foi a solucao mais plausivel
  if (tipoIdx != TIPO_INT) erroSemantico(linha, "indice de vetor precisa ser inteiro, em", nome);
  // se a expressao foi so um literal (1 token a mais lido), da pra checar o limite agora
  if (ehLit && contaTokens == tokensAntes + 1) {
    if (valLit < info->lo || valLit > info->hi) erroSemantico(linha, "indice fora dos limites do vetor", nome);
  }
  consome(TOKEN_DELIM, "]");
  emite_num("CRCT", info->lo);
  emite("SUBT");
  emite_nome("CREN", nome);
  emite("SOMA");
}

/*
SECAO 3: ANALISADOR LEXICO (SCANNER)
Interacao com o Parser conforme Figura 1: obterToken() / infoToken()
*/

// (feitos) TODO (ETAPA 2): Implementar a leitura do arquivo fonte para um buffer em
// memoria (char *buffer), considerando que "todos os lexemas no codigo fonte
// estao separados por um espaco" (simplificacao dada pelo descritivo).

// (feitos) TODO (ETAPA 2): Implementar tabela de palavras reservadas do MiniVisualg,
// extraidas dos exemplos do Anexo I, por exemplo:
//   algoritmo, var, inicio, fimalgoritmo, caractere, inteiro, real, logico,
//   vetor, de, escreva, escreval, leia, se, entao, senao, fimse, para, ate,
//   passo, faca, fimpara, enquanto, fimenquanto, procedimento,
//   fimprocedimento, funcao, fimfuncao, retorne, verdadeiro, falso, e, ou,
//   nao, mod
// (feitos) TODO: confirmar lista completa e definitiva junto da Etapa 1 (GLC).


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
  "E", "OU", "mod", "MOD", "nao"
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
(feitos) TODO (ETAPA 2): obterToken()
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
     - (feitos) TODO: demais simbolos da linguagem (operadores aritmeticos,
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
    
    // fix do print das palabras reservadas
    case TOKEN_KEYWORD: return "";
    
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
(feitos) TODO (ETAPA 2): infoToken()
Conforme Figura 1, e responsavel por fornecer ao Parser as informacoes do
token corrente (usada pelo PARSER via infoToken()).
Deve tambem cobrir a exigencia de impressao no formato:
  "Numero da Linha do Atomo# NomeToken | Atributo"
  Exemplo: "11# IDENTIFICADOR | 1"  ou  "11# ID | 1"
(feitos) TODO: garantir que a saida impressa na tela seja IDENTICA a saida gravada
no arquivo de tokens (mesma formatacao, mesma ordem).
*/
void infoToken(Token t) {
  char linhaFormatada[512];

  // correcao da prof sobre EOF de imprimir na tela/arquivo.
  if (t.type == TOKEN_EOF) {
    return;
  }
  // fix da palavra reservada 
  else if (t.type == TOKEN_KEYWORD) {
    snprintf(linhaFormatada, sizeof(linhaFormatada), "%d# %s\n", t.line, symtab[t.attribute.table_index]);
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
    // CONFIRMAR correcao da prof de novo rs... aqui era o numero do ID
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
(feitos) TODO (ETAPA 2): erroLexico()
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
(feitos) TODO (ETAPA 3): nextToken()
Conforme Figura 1 (PARSER contem "char lookahead" e nextToken()):
  1. Chamar obterToken() (do analisador lexico) para obter o proximo token.
  2. Armazenar o resultado na variavel global lookahead do parser.
*/
void nextToken(void) {
  lookahead = obterToken();
  contaTokens++; // (fase 2) saber se um indice de vetor foi so um literal
  infoToken(lookahead);
}

/*
(feitos) TODO (ETAPA 3): Implementar analise descendente (recursiva) com base na
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

(feitos) TODO: erroSintatico()
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
/*
TODO (FASE 2): analisar_programa().
  1. Preparar a geracao MEPA: INPP, reserva de memoria e desvio para
     o bloco principal, deixando as rotinas em uma parte separada.
  2. Ao final, gerar liberacao de memoria e PARA.
  3. O endereco ordinal de TNo nao e necessariamente o deslocamento MEPA.
*/

void analisar_programa(void) {
  consome(TOKEN_KEYWORD, "algoritmo");
  consome(TOKEN_STRING, NULL);
  // rotulo do bloco principal reservado antes das rotinas (elas pegam os proximos)
  rotuloMain = proximo_rotulo();
  analisar_subrotinas(); // daqui o fluxo vai pra analisar_subrotinas() e volta com o codigo das rotinas no buffer
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "var") == 0) {
    consome(TOKEN_KEYWORD, "var");
    analisar_declaracoes();
  }
  // a gramatica da etapa 1 manda as rotinas DEPOIS do var (teste18), mas o parser ja aceitava
  // antes tambem (testes 13 e 17), entao chama de novo aqui e aceita as duas ordens
  analisar_subrotinas();
  consome(TOKEN_KEYWORD, "inicio");
  // INPP, AMEM e o DSVS pra ca sao escritos no topo do arquivo pela grava_mepa(),
  // porque o total do AMEM so e conhecido no fim. daqui pra baixo e o corpo do main
  emite_rotulo(rotuloMain);
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimalgoritmo");
  // o DMEM e o PARA tambem ficam por conta da grava_mepa()
}
/*
TODO (FASE 2): analisar_declaracoes().
  1. Guardar os nomes da lista (ex: a, b, c) ate descobrir o tipo.
  2. Inserir cada nome na lista TNo e verificar declaracao repetida.
  3. Palavra reservada usada como nome deve gerar ERRO SEMANTICO.
  4. Calcular o espaco de variaveis e vetores para a MEPA.
*/

void analisar_declaracoes(void) {
  char nomes[32][256]; // nomes da lista ficam guardados ate descobrir o tipo
  int linhas[32];
  int n, i, tipo;
  int ehVetor, lo, hi;

  // a area de var acaba no "inicio" (ou no "procedimento"/"funcao" que vem depois); qualquer
  // outra palavra reservada aqui e uma palavra reservada usada como nome (ver le_nome_declaracao)
  while (lookahead.type == TOKEN_ID ||
         (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "inicio") != 0 &&
          strcmp(symtab[lookahead.attribute.table_index], "procedimento") != 0 &&
          strcmp(symtab[lookahead.attribute.table_index], "funcao") != 0)) {
    n = 0;
    le_nome_declaracao(nomes[n], &linhas[n]);
    n++;
    while (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
      consome(TOKEN_DELIM, ",");
      if (n >= 32) erroSemantico(lookahead.line, "nomes demais numa mesma declaracao", "");
      le_nome_declaracao(nomes[n], &linhas[n]);
      n++;
    }
    consome(TOKEN_DELIM, ":");
    tipo = analisar_tipo(); // daqui o fluxo vai pra analisar_tipo(), volta com o tipo (e limites em vetLo/vetHi)
    ehVetor = tipoEhVetor;
    lo = vetLo;
    hi = vetHi;
    // agora que o tipo apareceu, insere cada nome na minitabela
    for (i = 0; i < n; i++) {
      insere_variavel(nomes[i], linhas[i], tipo, ehVetor, lo, hi, 0);
    }
  }

  // correcao das palavras reservadas (tavam dando erro sint - devem dar erro sem)
  if (lookahead.type == TOKEN_KEYWORD) {
    long pos = bufPos;
    while (pos < bufLen) {
      if (isspace((unsigned char) buffer[pos])) {
        pos++;
      } else if (buffer[pos] == '/' && pos + 1 < bufLen && buffer[pos + 1] == '/') {
        while (pos < bufLen && buffer[pos] != '\n') pos++;
      } else {
        break;
      }
    }
    if (pos < bufLen && buffer[pos] == ':') {
      erroSemantico(lookahead.line, "palavra reservada nao pode ser nome de variavel", symtab[lookahead.attribute.table_index]);
    }
  }
}
/*
TODO (FASE 2): analisar_tipo().
  1. Identificar o tipo declarado e devolver essa informacao ao parser.
  2. Para vetor, guardar os limites e verificar se o intervalo e valido.
  3. Usar o tipo do elemento nas atribuicoes e acessos indexados.
  4. Manter a verificacao dos tipos real, caractere e logico, mesmo
     que a MEPA original nao consiga executar todos eles.
*/

int analisar_tipo(void) {
  int tipo = TIPO_NENHUM;
  int lo, hi, linha;
  tipoEhVetor = 0;
  if (lookahead.type != TOKEN_KEYWORD) erroSintatico(lookahead.line, lookahead);

  if (strcmp(symtab[lookahead.attribute.table_index], "caractere") == 0) {
    consome(TOKEN_KEYWORD, "caractere");
    tipo = TIPO_CHAR;
  } else if (strcmp(symtab[lookahead.attribute.table_index], "inteiro") == 0) {
    consome(TOKEN_KEYWORD, "inteiro");
    tipo = TIPO_INT;
  } else if (strcmp(symtab[lookahead.attribute.table_index], "real") == 0) {
    consome(TOKEN_KEYWORD, "real");
    tipo = TIPO_REAL;
  } else if (strcmp(symtab[lookahead.attribute.table_index], "logico") == 0) {
    consome(TOKEN_KEYWORD, "logico");
    tipo = TIPO_LOG;
  } else if (strcmp(symtab[lookahead.attribute.table_index], "vetor") == 0) {
    linha = lookahead.line;
    consome(TOKEN_KEYWORD, "vetor");
    consome(TOKEN_DELIM, "[");
    lo = lookahead.attribute.int_value; // le o valor antes do consome, depois o lookahead ja e outro
    consome(TOKEN_NUM_INT, NULL);
    consome(TOKEN_DELIM, "..");
    hi = lookahead.attribute.int_value;
    consome(TOKEN_NUM_INT, NULL);
    consome(TOKEN_DELIM, "]");
    consome(TOKEN_KEYWORD, "de");
    if (lo > hi) erroSemantico(linha, "intervalo de vetor invalido (inicio maior que o fim)", "");
    tipo = analisar_tipo(); // tipo do elemento (a chamada recursiva zera tipoEhVetor)
    if (tipoEhVetor == 1) erroSemantico(linha, "vetor de vetor nao e permitido", "");
    tipoEhVetor = 1;
    vetLo = lo;
    vetHi = hi;
  } else {
    erroSintatico(lookahead.line, lookahead);
  }
  return tipo;
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
/*
TODO (FASE 2): analisar_comando().
  1. Diferenciar atribuicao, chamada de procedimento e acesso a vetor.
  2. Verificar declaracao do identificador antes de usar seu valor.
  3. Na atribuicao, comparar os tipos e marcar a variavel como atribuida.
  4. Proibir funcao usada como procedimento e procedimento em expressao.
  5. Em retorne, conferir se esta dentro de funcao e comparar o tipo.
*/

void analisar_comando(void) {
  char nome[256];
  int linha, tipoExpr, idRot;
  TNo *no;
  InfoVar *info;

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
      linha = lookahead.line;
      consome(TOKEN_KEYWORD, "retorne");
      tipoExpr = analisar_expressao(); // daqui o fluxo vai pra analisar_expressao() e volta com o tipo
      if (rotinaAtual < 0 || rotinas[rotinaAtual].ehFuncao == 0) {
        erroSemantico(linha, "retorne so pode ser usado dentro de funcao", "");
      }
      if (!tipo_atribuivel(rotinas[rotinaAtual].tipoRet, tipoExpr)) {
        erro_tipos(linha, "retorne", rotinas[rotinaAtual].tipoRet, tipoExpr);
      }
      // guarda o valor no slot de retorno da funcao e pula pro RTPR (fim da rotina)
      emite_ret("ARMZ", rotinaAtual);
      emite_rot("DSVS", rotinas[rotinaAtual].rotuloFim);
    } else erroSintatico(lookahead.line, lookahead);
  } else if (lookahead.type == TOKEN_ID) {
    consome_id(nome);
    linha = linhaUltimoId;
    if (lookahead.type == TOKEN_ASSIGN) {
      // atribuicao simples: id <- expr
      no = no_declarado(nome);
      info = busca_info(no);
      if (info->ehVetor) erroSemantico(linha, "vetor precisa de indice", nome);
      consome(TOKEN_ASSIGN, NULL);
      tipoExpr = analisar_expressao();
      if (!tipo_atribuivel(info->tipo, tipoExpr)) erro_tipos(linha, "atribuicao", info->tipo, tipoExpr);
      marca_atribuida(info, linha);
      emite_nome("ARMZ", nome);
    } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
      // atribuicao em vetor: id[expr] <- expr
      no = no_declarado(nome);
      info = busca_info(no);
      if (info->ehVetor == 0) erroSemantico(linha, "identificador nao e um vetor", nome);
      gera_endereco_vetor(info, nome, linha); // endereco do elemento fica no topo da pilha
      usaTmpVet = 1;
      emite_nome("ARMZ", "$tv2"); // guarda o endereco ($tv2 e so das atribuicoes, $tv1 e das leituras)
      consome(TOKEN_ASSIGN, NULL);
      tipoExpr = analisar_expressao();
      if (!tipo_atribuivel(info->tipo, tipoExpr)) erro_tipos(linha, "atribuicao", info->tipo, tipoExpr);
      marca_atribuida(info, linha);
      emite_nome("ARMI", "$tv2");
    } else {
      // chamada de procedimento (com parenteses, ou sem se nao tiver parametro)
      idRot = busca_rotina(nome);
      if (idRot < 0) {
        no_declarado(nome); // se nem variavel for, a busca_tabela_simbolos() ja da o erro de nao declarado
        erroSemantico(linha, "identificador nao e um procedimento", nome);
      }
      if (rotinas[idRot].ehFuncao == 1) erroSemantico(linha, "funcao nao pode ser usada como procedimento", nome);
      chama_rotina(idRot, linha);
    }
  } else erroSintatico(lookahead.line, lookahead);
}
/*
TODO (FASE 2): analisar_leitura().
  1. Verificar se o identificador existe e pode receber um valor.
  2. Em vetor, verificar se o indice e inteiro.
  3. Gerar LEIT e o armazenamento adequado; marcar como atribuida.
*/

void analisar_leitura(void) {
    char nome[256];
    int linha;
    TNo *no;
    InfoVar *info;

    consome(TOKEN_KEYWORD, "leia");
    consome(TOKEN_DELIM, "(");
    emite("LEIT"); // o valor lido fica na pilha, o ARMZ/ARMI abaixo guarda na variavel
    consome_id(nome);
    linha = linhaUltimoId;
    no = no_declarado(nome);
    info = busca_info(no);
    if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
      if (info->ehVetor == 0) erroSemantico(linha, "identificador nao e um vetor", nome);
      gera_endereco_vetor(info, nome, linha);
      usaTmpVet = 1;
      emite_nome("ARMZ", "$tv2");
      emite_nome("ARMI", "$tv2");
    } else {
      if (info->ehVetor) erroSemantico(linha, "vetor precisa de indice", nome);
      emite_nome("ARMZ", nome);
    }
    // a LEIT da mepa so le inteiro
    if (info->tipo == TIPO_REAL || info->tipo == TIPO_CHAR) marca_sem_mepa("leitura de real/caractere nao e suportada pela MEPA");
    marca_atribuida(info, linha);
    consome(TOKEN_DELIM, ")");
}
/*
TODO (FASE 2): analisar_escrita().
  1. Analisar cada argumento, verificando seu tipo.
  2. Gerar o valor da expressao e a instrucao IMPR para cada argumento.
  3. Definir como tratar strings e reais na MEPA original.
*/

void analisar_escrita(void) {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "escreva") == 0) {
      consome(TOKEN_KEYWORD, "escreva");
    } else if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "escreval") == 0) {
      consome(TOKEN_KEYWORD, "escreval");
    } else {
      erroSintatico(lookahead.line, lookahead);
    }
    consome(TOKEN_DELIM, "(");
    analisar_lista_argumentos(-1); // -1 = argumentos do escreva, nao de rotina (gera um IMPR por argumento)
    consome(TOKEN_DELIM, ")");
}
/*
TODO (FASE 2): analisar_lista_argumentos().
  1. Contar os argumentos informados na chamada.
  2. Comparar quantidade e tipo com os parametros declarados.
  3. Preparar a passagem por valor e a chamada CHPR na MEPA.
*/

void analisar_lista_argumentos(int idRot) {
    int n = 0;
    int linha, tipo;

    // chamada sem argumentos f() (so pra rotina, no escreva continua dando erro sintatico)
    if (idRot >= 0 && lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ")") == 0) {
      if (rotinas[idRot].nParam != 0) erroSemantico(lookahead.line, "numero de argumentos incompativel na chamada de", rotinas[idRot].nome);
      return;
    }
    for (;;) {
      linha = lookahead.line;
      tipo = analisar_expressao();
      if (idRot < 0) {
        emite("IMPR"); // imprime o valor que a expressao deixou na pilha
      } else {
        // confere quantidade e tipo de cada argumento
        if (n >= rotinas[idRot].nParam) erroSemantico(linha, "numero de argumentos incompativel na chamada de", rotinas[idRot].nome);
        if (!tipo_atribuivel(rotinas[idRot].tipoParam[n], tipo)) erro_tipos(linha, "argumento da chamada", rotinas[idRot].tipoParam[n], tipo);
      }
      n++;
      if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
        consome(TOKEN_DELIM, ",");
      } else {
        break;
      }
    }
    if (idRot >= 0 && n != rotinas[idRot].nParam) {
      erroSemantico(lookahead.line, "numero de argumentos incompativel na chamada de", rotinas[idRot].nome);
    }
}
/*
TODO (FASE 2): analisar_expressao().
  Fazer a analise retornar o tipo da expressao ao chamador.
  Usar essa informacao para conferir atribuicoes, condicoes,
  indices de vetores, argumentos e retorno de funcoes.
*/

int analisar_expressao(void) {
  return analisar_expr_logica();
}
/*
TODO (FASE 2): analisar_expr_logica().
  1. Exigir operandos logicos em E e OU.
  2. Gerar CONJ ou DISJ, com resultado logico.
  3. Revisar a precedencia: a aula apresenta E antes de OU;
     o parser atual trata ambos na mesma repeticao.
*/

int analisar_expr_logica(void) {
  int tipo, tipo2, linha;
  tipo = analisar_expr_e(); // OU tem prioridade menor, entao o operando dele e uma expr com E
  while (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "OU") == 0) {
    linha = lookahead.line;
    consome(TOKEN_KEYWORD, "OU");
    tipo2 = analisar_expr_e();
    if (tipo != TIPO_LOG || tipo2 != TIPO_LOG) erro_tipos(linha, "operador OU (precisa de logicos)", tipo, tipo2);
    emite("DISJ");
    tipo = TIPO_LOG;
  }
  return tipo;
}

// nivel do E (prioridade maior que o OU, como na aula)
int analisar_expr_e(void) {
  int tipo, tipo2, linha;
  tipo = analisar_expr_relacional();
  while (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "E") == 0) {
    linha = lookahead.line;
    consome(TOKEN_KEYWORD, "E");
    tipo2 = analisar_expr_relacional();
    if (tipo != TIPO_LOG || tipo2 != TIPO_LOG) erro_tipos(linha, "operador E (precisa de logicos)", tipo, tipo2);
    emite("CONJ");
    tipo = TIPO_LOG;
  }
  return tipo;
}
/*
TODO (FASE 2): analisar_expr_relacional().
  1. Conferir tipos compativeis nas comparacoes.
  2. O resultado da comparacao e logico.
  3. Emitir a instrucao MEPA correspondente ao operador.
*/

int analisar_expr_relacional(void) {
  int tipo, tipo2, linha, op, ok;
  tipo = analisar_expr_aritmetica();
  if (lookahead.type == TOKEN_OP_REL) {
    linha = lookahead.line;
    op = lookahead.attribute.op_code;
    consome(TOKEN_OP_REL, NULL);
    tipo2 = analisar_expr_aritmetica();

    ok = 0;
    if (tipo_numerico(tipo) && tipo_numerico(tipo2)) ok = 1; // inteiro e real se comparam
    if (tipo == tipo2 && tipo == TIPO_CHAR) ok = 1;
    if (tipo == tipo2 && tipo == TIPO_LOG && (op == OP_EQ || op == OP_NE)) ok = 1; // logico so = e <>
    if (ok == 0) erro_tipos(linha, "comparacao", tipo, tipo2);
    if (tipo == TIPO_CHAR) marca_sem_mepa("comparacao de caractere nao e suportada pela MEPA");

    switch (op) {
      case OP_LT: emite("CMME"); break;
      case OP_LE: emite("CMEG"); break;
      case OP_EQ: emite("CMIG"); break;
      case OP_GT: emite("CMMA"); break;
      case OP_GE: emite("CMAG"); break;
      case OP_NE: emite("CMDG"); break;
    }
    tipo = TIPO_LOG;
  }
  return tipo;
}
/*
TODO (FASE 2): analisar_expr_aritmetica().
  Verificar tipos em + e - e emitir SOMA ou SUBT.
  Aceitar combinacoes numericas conforme as regras de tipos adotadas.
*/

int analisar_expr_aritmetica(void) {
  int tipo, tipo2, linha, soma;
  tipo = analisar_termo();
  while (lookahead.type == TOKEN_OP_ARIT && (strcmp(symtab[lookahead.attribute.table_index], "+") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "-") == 0)) {
    linha = lookahead.line;
    if (strcmp(symtab[lookahead.attribute.table_index], "+") == 0) {
      soma = 1;
      consome(TOKEN_OP_ARIT, "+");
    } else {
      soma = 0;
      consome(TOKEN_OP_ARIT, "-");
    }
    tipo2 = analisar_termo();
    if (!tipo_numerico(tipo) || !tipo_numerico(tipo2)) erro_tipos(linha, "soma/subtracao", tipo, tipo2);
    tipo = tipo_aritmetico(tipo, tipo2);
    if (soma == 1) emite("SOMA");
    else emite("SUBT");
  }
  return tipo;
}
/*
TODO (FASE 2): analisar_termo().
  1. Verificar os tipos em *, /, \ e MOD.
  2. Gerar MULT e DIVI quando os operandos forem inteiros.
  3. A MEPA original nao tem MOD; traduzir a operacao usando
     subtracao, divisao e multiplicacao sem perder operandos.
*/

int analisar_termo(void) {
  int tipo, tipo2, linha, op; // op: 1 = *, 2 = /, 3 = \, 4 = mod
  tipo = analisar_fator();
  while ((lookahead.type == TOKEN_OP_ARIT && 
    (strcmp(symtab[lookahead.attribute.table_index], "*") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "/") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "\\") == 0)) 
    ||
    (lookahead.type == TOKEN_KEYWORD && 
    (strcmp(symtab[lookahead.attribute.table_index], "MOD") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "mod") == 0))) {

    linha = lookahead.line;
    op = 4;
    if (lookahead.type == TOKEN_OP_ARIT) {
      if (strcmp(symtab[lookahead.attribute.table_index], "*") == 0) {
        op = 1;
        consome(TOKEN_OP_ARIT, "*");
      } else if (strcmp(symtab[lookahead.attribute.table_index], "/") == 0) {
        op = 2;
        consome(TOKEN_OP_ARIT, "/");
      } else {
        op = 3;
        consome(TOKEN_OP_ARIT, "\\");
      }
    } else {
      if (strcmp(symtab[lookahead.attribute.table_index], "MOD") == 0) {
        consome(TOKEN_KEYWORD, "MOD");
      } else {
        consome(TOKEN_KEYWORD, "mod");
      }
    }
    tipo2 = analisar_fator();

    if (op == 1) {
      if (!tipo_numerico(tipo) || !tipo_numerico(tipo2)) erro_tipos(linha, "multiplicacao", tipo, tipo2);
      tipo = tipo_aritmetico(tipo, tipo2);
      emite("MULT");
    } else if (op == 2) {
      // padrao minivisualg: / e divisao REAL (o resultado e real mesmo com dois inteiros)
      if (!tipo_numerico(tipo) || !tipo_numerico(tipo2)) erro_tipos(linha, "divisao", tipo, tipo2);
      tipo = TIPO_REAL;
      marca_sem_mepa("divisao com / gera valor real, que a MEPA nao suporta");
    } else if (op == 3) {
      // \ e divisao inteira: so inteiros, resultado inteiro, vira DIVI
      if (tipo != TIPO_INT || tipo2 != TIPO_INT) erro_tipos(linha, "divisao inteira", tipo, tipo2);
      tipo = TIPO_INT;
      emite("DIVI");
    } else {
      if (tipo != TIPO_INT || tipo2 != TIPO_INT) erro_tipos(linha, "mod", tipo, tipo2);
      tipo = TIPO_INT;
      gera_mod();
    }
  }
  return tipo;
}
/*
TODO (FASE 2): analisar_fator().
  1. Reconhecer o tipo de constantes, variaveis e chamadas.
  2. Marcar como usada a variavel lida em uma expressao.
  3. Proibir procedimento no lugar de um valor.
  4. Em vetor, exigir indice inteiro e gerar acesso ao elemento.
  5. Gerar CRCT para inteiros e logicos, CRVL para variaveis
     e INVR para menos unario.
*/

int analisar_fator(void) {
  char nome[256];
  int tipo = TIPO_INT;
  int linha, idRot, negativo;
  TNo *no;
  InfoVar *info;

  // nao e reservado e opera so sobre logico; daqui volta pro fator seguinte
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "nao") == 0) {
    linha = lookahead.line;
    consome(TOKEN_KEYWORD, "nao");
    tipo = analisar_fator();
    if (tipo != TIPO_LOG) erroSemantico(linha, "operador nao exige valor logico", "");
    emite("NEGA");
    return TIPO_LOG;
  }

  if (lookahead.type == TOKEN_OP_ARIT && (strcmp(symtab[lookahead.attribute.table_index], "+") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "-") == 0)) {
    linha = lookahead.line;
    negativo = 0;
    if (strcmp(symtab[lookahead.attribute.table_index], "+") == 0) {
      consome(TOKEN_OP_ARIT, "+");
    } else {
      negativo = 1;
      consome(TOKEN_OP_ARIT, "-");
    }
    tipo = analisar_fator();
    if (!tipo_numerico(tipo)) erroSemantico(linha, "sinal + ou - so pode ser usado com numero", "");
    if (negativo == 1) emite("INVR");
    return tipo;
  }
  
  if (lookahead.type == TOKEN_NUM_INT) {
    emite_num("CRCT", lookahead.attribute.int_value); // pega o valor antes do consome
    consome(TOKEN_NUM_INT, NULL);
    tipo = TIPO_INT;
  } else if (lookahead.type == TOKEN_NUM_FLOAT) {
    consome(TOKEN_NUM_FLOAT, NULL);
    tipo = TIPO_REAL; // sem codigo: a mepa nao tem real
  } else if (lookahead.type == TOKEN_STRING) {
    consome(TOKEN_STRING, NULL);
    tipo = TIPO_CHAR; // sem codigo: a mepa nao tem string
  } else if (lookahead.type == TOKEN_KEYWORD && (strcmp(symtab[lookahead.attribute.table_index], "verdadeiro") == 0 || 
    strcmp(symtab[lookahead.attribute.table_index], "falso") == 0)) {
    if (strcmp(symtab[lookahead.attribute.table_index], "verdadeiro") == 0) {
      emite_num("CRCT", 1);
      consome(TOKEN_KEYWORD, "verdadeiro");
    } else {
      emite_num("CRCT", 0);
      consome(TOKEN_KEYWORD, "falso");
    }
    tipo = TIPO_LOG;
  } else if (lookahead.type == TOKEN_ID) {
    consome_id(nome);
    linha = linhaUltimoId;
    idRot = busca_rotina(nome);
    if (idRot >= 0) {
      // chamada de funcao dentro de expressao (procedimento aqui e erro)
      if (rotinas[idRot].ehFuncao == 0) erroSemantico(linha, "procedimento nao pode ser usado em expressao", nome);
      if (!(lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0)) {
        erroSemantico(linha, "chamada de funcao precisa de parenteses", nome);
      }
      chama_rotina(idRot, linha); // daqui o fluxo vai pra chama_rotina() -> analisar_lista_argumentos()
      emite_ret("CRVL", idRot); // valor de retorno fica no slot da funcao, empilha ele
      tipo = rotinas[idRot].tipoRet;
    } else {
      no = no_declarado(nome);
      info = busca_info(no);
      info->usada = 1;
      tipo = info->tipo;
      if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "[") == 0) {
        if (info->ehVetor == 0) erroSemantico(linha, "identificador nao e um vetor", nome);
        gera_endereco_vetor(info, nome, linha);
        usaTmpVet = 1;
        emite_nome("ARMZ", "$tv1"); // CRVI precisa do endereco guardado numa variavel
        emite_nome("CRVI", "$tv1");
      } else {
        if (info->ehVetor) erroSemantico(linha, "vetor precisa de indice", nome);
        emite_nome("CRVL", nome);
      }
    }
  } else if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
    consome(TOKEN_DELIM, "(");
    tipo = analisar_expressao();
    consome(TOKEN_DELIM, ")");
  } else {
    erroSintatico(lookahead.line, lookahead);
  }

  // qualquer real ou caractere/string na expressao: a mepa original nao roda
  if (tipo == TIPO_REAL) marca_sem_mepa("uso de tipo real, que a MEPA nao suporta");
  if (tipo == TIPO_CHAR) marca_sem_mepa("uso de caractere/string, que a MEPA nao suporta");
  return tipo;
}
/*
TODO (FASE 2): analisar_condicional().
  1. Exigir condicao logica.
  2. Gerar DSVF para o senao (ou fim) e DSVS para o fim.
  3. Colocar os rotulos nas posicoes corretas do codigo.
*/

void analisar_condicional(void) {
  int linha, tipo, l1, l2;
  // rotulos pegos antes dos blocos (os blocos chamam proximo_rotulo() tambem)
  l1 = proximo_rotulo();
  l2 = proximo_rotulo();
  consome(TOKEN_KEYWORD, "se");
  consome(TOKEN_DELIM, "(");
  linha = lookahead.line;
  tipo = analisar_expressao();
  if (tipo != TIPO_LOG) erroSemantico(linha, "condicao do se precisa ser logica", "");
  consome(TOKEN_DELIM, ")");
  emite_rot("DSVF", l1); // falso: vai pro senao (ou pro fim)
  consome(TOKEN_KEYWORD, "entao");
  analisar_bloco_comandos();
  emite_rot("DSVS", l2); // acabou o bloco do entao: pula o senao
  emite_rotulo(l1);
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "senao") == 0) {
    consome(TOKEN_KEYWORD, "senao");
    analisar_bloco_comandos();
  }
  emite_rotulo(l2);
  consome(TOKEN_KEYWORD, "fimse");
}
/*
TODO (FASE 2): analisar_repeticao_enquanto().
  1. Criar rotulo para o teste e outro para a saida.
  2. Exigir condicao logica e gerar DSVF para encerrar.
  3. Gerar o corpo e DSVS para repetir o teste.
*/

void analisar_repeticao_enquanto(void) {
  int linha, tipo, l1, l2;
  l1 = proximo_rotulo(); // teste
  l2 = proximo_rotulo(); // saida
  consome(TOKEN_KEYWORD, "enquanto");
  emite_rotulo(l1);
  consome(TOKEN_DELIM, "(");
  linha = lookahead.line;
  tipo = analisar_expressao();
  if (tipo != TIPO_LOG) erroSemantico(linha, "condicao do enquanto precisa ser logica", "");
  consome(TOKEN_DELIM, ")");
  emite_rot("DSVF", l2);
  consome(TOKEN_KEYWORD, "faca");
  analisar_bloco_comandos();
  emite_rot("DSVS", l1);
  emite_rotulo(l2);
  consome(TOKEN_KEYWORD, "fimenquanto");
}
/*
TODO (FASE 2): analisar_repeticao_para().
  1. Exigir variavel de controle inteira e declarada.
  2. Verificar tipos de inicio, limite e passo; impedir passo zero.
  3. Gerar inicializacao, teste, corpo e incremento.
  4. O incremento deve terminar em ARMZ, pois SOMA so deixa
     o novo valor na pilha (correcao discutida na aula).
  5. Tratar passos negativos com a comparacao apropriada.
*/

void analisar_repeticao_para(void) {
  char nome[256];
  int linha, tipo, rotTeste, rotFim, passo, negativo, idxCmp;
  TNo *no;
  InfoVar *info;

  rotTeste = proximo_rotulo();
  rotFim = proximo_rotulo();
  consome(TOKEN_KEYWORD, "para");
  consome_id(nome);
  linha = linhaUltimoId;
  no = no_declarado(nome);
  info = busca_info(no);
  if (info->ehVetor || info->tipo != TIPO_INT) erroSemantico(linha, "variavel de controle do para precisa ser inteira", nome);
  info->usada = 1; // o proprio para le a variavel no teste

  consome(TOKEN_KEYWORD, "de");
  tipo = analisar_expressao();
  if (tipo != TIPO_INT) erro_tipos(linha, "inicio do para", TIPO_INT, tipo);
  emite_nome("ARMZ", nome); // i <- inicio
  emite_rotulo(rotTeste);
  emite_nome("CRVL", nome);
  consome(TOKEN_KEYWORD, "ate");
  tipo = analisar_expressao();
  if (tipo != TIPO_INT) erro_tipos(linha, "limite do para", TIPO_INT, tipo);

  // comparacao fica como placeholder: CMEG (i <= limite). se o passo for negativo
  // troca por CMAG (i >= limite) la embaixo, quando o passo ja foi lido
  idxCmp = nCod;
  emite("CMEG");

  passo = 1;
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "passo") == 0) {
    consome(TOKEN_KEYWORD, "passo");
    negativo = 0;
    if (lookahead.type == TOKEN_OP_ARIT && strcmp(symtab[lookahead.attribute.table_index], "-") == 0) {
      negativo = 1;
      consome(TOKEN_OP_ARIT, "-");
    } else if (lookahead.type == TOKEN_OP_ARIT && strcmp(symtab[lookahead.attribute.table_index], "+") == 0) {
      consome(TOKEN_OP_ARIT, "+");
    }
    // simplificacao: o passo precisa ser constante inteira (assim o sinal e conhecido na compilacao)
    if (lookahead.type != TOKEN_NUM_INT) erroSemantico(lookahead.line, "passo do para precisa ser uma constante inteira", "");
    passo = lookahead.attribute.int_value;
    if (negativo == 1) passo = -passo;
    if (passo == 0) erroSemantico(lookahead.line, "passo do para nao pode ser zero", "");
    consome(TOKEN_NUM_INT, NULL);
  }
  if (passo < 0) strcpy(codigo[idxCmp], "CMAG");

  emite_rot("DSVF", rotFim);
  consome(TOKEN_KEYWORD, "faca");
  analisar_bloco_comandos();
  // incremento: i <- i + passo. termina em ARMZ porque o SOMA so deixa o valor na pilha
  emite_nome("CRVL", nome);
  emite_num("CRCT", passo);
  emite("SOMA");
  emite_nome("ARMZ", nome);
  emite_rot("DSVS", rotTeste);
  emite_rotulo(rotFim);
  consome(TOKEN_KEYWORD, "fimpara");
}
/*
TODO (FASE 2): analisar_subrotinas().
  Guardar os dados das funcoes e procedimentos e gerar seus corpos
  em trechos separados do corpo principal do programa.
*/

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
/*
TODO (FASE 2): analisar_declaracao_procedimento().
  1. Cadastrar o nome da rotina e os tipos dos parametros.
  2. Impedir nomes repetidos com variaveis ou outras rotinas.
  3. Preparar ENPR, memoria local e RTPR no codigo MEPA.
  4. Registrar a rotina antes do corpo para permitir recursao direta.
*/

void analisar_declaracao_procedimento(void) {
  char nome[256];
  int linha, idRot;
  consome(TOKEN_KEYWORD, "procedimento");
  consome_id(nome);
  linha = linhaUltimoId;
  idRot = registra_rotina(nome, linha, 0); // cadastra antes do corpo (recursao direta)
  rotinaAtual = idRot;
  emite_rotulo(rotinas[idRot].rotulo);
  emite_num("ENPR", 1);
  if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], "(") == 0) {
    consome(TOKEN_DELIM, "(");
    // fix do procedimento sem params (correcao do anexo 1)
    if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ")") == 0) erroSintatico(lookahead.line, lookahead);
    analisar_lista_parametros(); // daqui o fluxo vai pra analisar_lista_parametros() e volta
    consome(TOKEN_DELIM, ")");
  }
  // gramatica da etapa 1: o procedimento tambem pode ter var local (teste18)
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "var") == 0) {
    consome(TOKEN_KEYWORD, "var");
    analisar_declaracoes();
  }
  consome(TOKEN_KEYWORD, "inicio");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimprocedimento");
  emite_rotulo(rotinas[idRot].rotuloFim);
  emite("RTPR 1, 0"); // nao tem variavel local na pilha (alocacao estatica), entao n = 0
  rotinaAtual = -1;
}
/*
TODO (FASE 2): analisar_lista_parametros().
  Registrar nome e tipo de cada parametro para verificar chamadas.
  Guardar sua posicao no registro de ativacao da rotina.
*/

void analisar_lista_parametros(void) {
  char nome[256];
  int linha, tipo, n;

  // lista vazia: procedimento p() / funcao f()
  if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ")") == 0) return;

  for (;;) {
    le_nome_declaracao(nome, &linha);
    consome(TOKEN_DELIM, ":");
    tipo = analisar_tipo();
    if (tipoEhVetor) erroSemantico(linha, "parametro nao pode ser vetor", nome);
    n = rotinas[rotinaAtual].nParam;
    if (n >= MAX_PARAMS) erroSemantico(linha, "parametros demais na rotina", nome);
    // parametro tambem entra na minitabela (endereco = ordem de declaracao) e nunca da erro de "nao usada"
    insere_variavel(nome, linha, tipo, 0, 0, 0, 1);
    rotinas[rotinaAtual].tipoParam[n] = tipo;
    strcpy(rotinas[rotinaAtual].nomeParam[n], nome);
    rotinas[rotinaAtual].nParam = n + 1;
    if (lookahead.type == TOKEN_DELIM && strcmp(symtab[lookahead.attribute.table_index], ",") == 0) {
      consome(TOKEN_DELIM, ",");
    } else {
      break;
    }
  }
}
/*
TODO (FASE 2): analisar_declaracao_funcao().
  1. Cadastrar o nome, os parametros e o tipo de retorno.
  2. Validar as expressoes de retorne dentro da funcao.
  3. Reservar o valor de retorno na chamada e gerar RTPR na saida.
  4. Registrar a funcao antes do corpo para permitir recursao direta.
*/

void analisar_declaracao_funcao(void) {
  char nome[256];
  int linha, idRot, tipo;
  consome(TOKEN_KEYWORD, "funcao");
  consome_id(nome);
  linha = linhaUltimoId;
  idRot = registra_rotina(nome, linha, 1); // cadastra antes do corpo (recursao direta)
  rotinaAtual = idRot;
  emite_rotulo(rotinas[idRot].rotulo);
  emite_num("ENPR", 1);
  consome(TOKEN_DELIM, "(");
  analisar_lista_parametros();
  consome(TOKEN_DELIM, ")");
  consome(TOKEN_DELIM, ":");
  tipo = analisar_tipo();
  if (tipoEhVetor) erroSemantico(linha, "funcao nao pode retornar vetor", nome);
  rotinas[idRot].tipoRet = tipo;
  // fix: aceita var locais da funcao, como na gramatica
  if (lookahead.type == TOKEN_KEYWORD && strcmp(symtab[lookahead.attribute.table_index], "var") == 0) {
    consome(TOKEN_KEYWORD, "var");
    analisar_declaracoes();
  }
  consome(TOKEN_KEYWORD, "inicio");
  analisar_bloco_comandos();
  consome(TOKEN_KEYWORD, "fimfuncao");
  emite_rotulo(rotinas[idRot].rotuloFim);
  emite("RTPR 1, 0"); // o retorne ja guardou o valor no slot $ret_nome
  rotinaAtual = -1;
}

/*
TODO (FASE 2): Integrar os erros semanticos sem mudar erroSintatico().
  O erro lexico continua com erroLexico() e o sintatico continua
  com erroSintatico(). Regras de tipos, nomes e rotinas devem
  produzir mensagem propria de erro semantico.
*/

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

/*
grava o mepa_cod.txt. so e chamada quando nao teve erro (lexico, sintatico ou semantico).
topo do arquivo: INPP, AMEM (total de posicoes) e DSVS pro bloco principal; fim: DMEM e PARA.

ADAPTACAO DA MEPA (o enunciado da parte 3 pede variaveis referenciadas pelo NOME no txt):
as instrucoes saem como "CRVL a" / "ARMZ b", e o topo do arquivo tem linhas de comentario
"; VAR nome endereco". o leComandosMepa() do mepa.c deve ler essas linhas e troca nome por
"nivel 0, endereco". as 34 instrucoes da mepa nao mudam.
*/
static void grava_mepa(void) {
  FILE *arq;
  TNo *no;
  int i, extra, total, base;
  int posMepa = 0;
  int iVar = 0;

  if (semMepa == 1) {
    //fprintf(stderr, "AVISO: %s; mepa_cod.txt nao foi gerado\n", motivoSemMepa);
    return;
  }

  // posicoes escondidas depois das variaveis: retorno de cada funcao e temporarios
  extra = 0;
  for (i = 0; i < nRotinas; i++) {
    if (rotinas[i].ehFuncao == 1) extra++;
  }
  if (usaTmpVet == 1) extra = extra + 2;
  if (usaTmpMod == 1) extra = extra + 2;
  total = proxEndereco + extra;

  // limites da mepa: 4096 instrucoes e 4096 posicoes de memoria
  if (nCod + 5 > 4096 || total > 4000) {
    fprintf(stderr, "AVISO: programa passa do limite da MEPA (4096 instrucoes ou posicoes); mepa_cod.txt nao foi gerado\n");
    return;
  }

  arq = fopen("mepa_cod.txt", "w");
  if (arq == NULL) {
    fprintf(stderr, "Erro ao criar mepa_cod.txt\n");
    return;
  }

  //; codigo intermediario MEPA gerado pelo compilador (fase 2)
  //; as linhas '; VAR nome endereco' sao lidas pelo leComandosMepa() do mepa.c
  no = minitabela;
  while (no != NULL) {
    // aqui vai a posicao da mepa, nao o endereco ordinal guardado na TNo
    fprintf(arq, "; VAR %s %d\n", no->ID, posMepa);
    // vetor ocupa varias casas na mepa, mas so um endereco ordinal na TNo
    if (infoVar[iVar].ehVetor == 1) {
      posMepa = posMepa + infoVar[iVar].hi - infoVar[iVar].lo + 1;
    } else {
      posMepa++;
    }
    iVar++;
    no = no->prox;
  }
  base = proxEndereco;
  for (i = 0; i < nRotinas; i++) {
    if (rotinas[i].ehFuncao == 1) {
      fprintf(arq, "; VAR $ret_%s %d\n", rotinas[i].nome, base);
      base++;
    }
  }
  if (usaTmpVet == 1) {
    fprintf(arq, "; VAR $tv1 %d\n", base);
    fprintf(arq, "; VAR $tv2 %d\n", base + 1);
    base = base + 2;
  }
  if (usaTmpMod == 1) {
    fprintf(arq, "; VAR $tma %d\n", base);
    fprintf(arq, "; VAR $tmb %d\n", base + 1);
  }

  fprintf(arq, "INPP\n");
  fprintf(arq, "AMEM %d\n", total);
  fprintf(arq, "DSVS L%d\n", rotuloMain); // pula as rotinas e vai pro bloco principal
  for (i = 0; i < nCod; i++) {
    fprintf(arq, "%s\n", codigo[i]);
  }
  fprintf(arq, "DMEM %d\n", total);
  fprintf(arq, "PARA\n");
  fclose(arq);
}

//SECAO 5: FUNCAO PRINCIPAL


/*
(feitos) TODO (ETAPA 2/3): main()
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
/*
TODO (FASE 2): main() e arquivo mepa_cod.txt.
  1. Remover codigo MEPA de uma compilacao anterior antes de comecar.
  2. Montar as instrucoes somente para o programa sem erros.
  3. Salvar mepa_cod.txt no fim, para nao deixar codigo incompleto.
  4. Conferir variaveis atribuidas e nunca usadas (menos parametros).
  5. Preservar tokens_saida.txt e a saida da Fase 1.
  6. Se houver tipo nao suportado pela MEPA original, avisar no stderr
     e nao gerar codigo; essa restricao precisa ser confirmada, pois
     o enunciado exige geracao para todos os comandos da linguagem.
*/

int main(int argc, char *argv[]) {
    TNo *proxNo;

    // apaga o codigo MEPA de uma compilacao anterior antes de comecar
    remove("mepa_cod.txt");

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

    // fase 2: so chega aqui se nao teve erro lexico/sintatico. agora falta a regra da
    // variavel atribuida e nao usada (erro semantico) e a gravacao do mepa_cod.txt
    confere_variaveis_nao_usadas();
    grava_mepa();

    // Liberacao dos recursos
    fclose(arqSaida);

    for (int i = 0; i < symCount; i++) {
        free(symtab[i]);
    }
    free(symtab);

    // libera os nos da minitabela (lista encadeada)
    while (minitabela != NULL) {
        proxNo = minitabela->prox;
        free(minitabela);
        minitabela = proxNo;
    }

    // correcao da prof sobre o padrao ID: idtab aponta para os mesmos
    // lexemas da symtab, que ja foram liberados acima.
    free(idtab);

    free(buffer);

    return 0;
}
/*
TODO (FASE 2): Conferencia final com o enunciado.
  1. Conferir declaracao previa, unicidade do nome e palavras reservadas.
  2. Conferir variavel atribuida e nao usada, tipos e assinaturas de rotinas.
  3. Conferir funcao em expressao, procedimento como comando e retornos.
  4. Conferir o formato do TXT: o enunciado pede nomes de variaveis,
     enquanto a MEPA usa nivel e deslocamento para acessar memoria.
     Definir uma representacao compativel sem confundir os enderecos.
  5. Validar memoria e quantidade maxima de instrucoes da MEPA.
  6. Reexecutar os testes da Fase 1 antes dos testes de codigo gerado.
*/