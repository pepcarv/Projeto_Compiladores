/* =====================================================================
mepa.c  -  Interpretador MEPA em um unico arquivo

URLs - MEPA

https://www.inf.ufpr.br/bmuller/assets/ci1211/Recursos/InterprMEPA-2.0.tar.bz2
 
Base: InterprMEPA-2.0, de Bruno Muller Junior (Set/2020), Licenca GNU GPL.
Os modulos MV_mepa, interface_usuario e executaMepa foram reunidos aqui
SEM alteracao de comportamento. Falta completar o leitor de arquivos
MEPA em C (secao LEITOR), que substituira leMepa.l e leMepa.y
para dispensar flex e bison.

Compilar: gcc -g -Og -Wall mepa.c -o mepa
Executar: ./mepa -r -i mepa_cod.txt      (-r = nao interativo)

Os TODOs abaixo indicam as partes que ainda faltam implementar.

TODO[M-06]: o codigo da professora, reunido sem mudancas, gera estes avisos
  com -Wall (nenhum e erro): variaveis nao usadas (k, i, linha, linha_out),
  retorno de scanf ignorado, 'rtpr' fora do switch que so imprime o estado
  interativo e 'cmd' possivelmente nao inicializada em executaMepa()
  (em modo -r ela e lida no while sem ter sido atribuida; inicializar com
  'proximo'). Corrigir somente o necessario para compilar sem avisos.
===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* ===================== MV_mepa.h ===================== */



/* -------------------------------------------------------------------
 *            Arquivo: MV_mepa.h
 * -------------------------------------------------------------------
 *              Autor: Bruno Muller Junior
 *               Data: 09/2020
 *      Atualizado em: [20/09/2020, 09h:46m]
 *
 * -------------------------------------------------------------------
 * A MEPA é uma máquina virtual contendo 5 estruturas: M, D, I, i, s.
 * Este arquivo contém as estruturas, variáveis globais e procedimentos
 * para acesso as cinco estruturas da máquina virtual.
 * ------------------------------------------------------------------- */




// ------------------------------------------------------------------------
// Declaração de Constantes
// ------------------------------------------------------------------------

#define TAM_M  4096     // Tamanho do Vetor Pilha da MV_mepa
#define TAM_RB 16       // Tamanho do Vetor Reg. Base da MV_mepa
#define TAM_I  4096     // Tamanho do Vetor Instruções da MV_mepa

typedef enum instrucao_mepa { 
  amem,   armi,   armz,   chpr,   cmag,   cmdg,   cmeg,   cmig,   cmma,
  cmme,   conj,   crct,   cren,   crvi,   crvl,   disj,   divi,   dmem,
  dsvf,   dsvr,   dsvs,   enpr,   enrt,   impr,   inpp,  invr,   leit,
  mult,   nada,   nega,   para,   rtpr,   soma,   subt
} instrucao_mepa;

// ------------------------------------------------------------------------
// Estrutura contida no vetor de instrucoes.
// ------------------------------------------------------------------------
typedef struct instStruct {
  instrucao_mepa instr;   // código de operação
  int op1;          // operando 1 
  int op2;          // operando 2
  int op3;          // operando 3
  int endDesvio;    // indice do desvio no vetor de instrucoes
  // --------- rotulo e rotDdesvio sao copias do arquivo de entrada
  char rotulo[10];  // rotulo associado a esta linha (se não houver, NULL)
  char rotDdesvio[10]; // o rotulo para onde desviar por exemplo dsvs (ou NULL)
} instStruct;


typedef struct MV_mepa {
  int vetorPilha[TAM_M];
  int vetorRegBase[TAM_RB];
  instStruct vetorInstr[TAM_I];
  int tam_i;
  int i;
  int s;
} MV_mepa;

// ------------------------------------------------------------------------
// Declaração de Variáveis Globais
// ------------------------------------------------------------------------
extern MV_mepa mv_mepa;


// ------------------------------------------------------------------------
// Declaração de Sinônimos
// ------------------------------------------------------------------------
#define mv_mepa_M      mv_mepa.vetorPilha
#define mv_mepa_D      mv_mepa.vetorRegBase
#define mv_mepa_I      mv_mepa.vetorInstr
#define mv_mepa_i      mv_mepa.i
#define mv_mepa_s      mv_mepa.s
#define mv_mepa_tam_i  mv_mepa.tam_i
#define mv_instr_atual mv_mepa_I[mv_mepa_i].instr
#define mv_instr_op1   mv_mepa_I[mv_mepa_i].op1
#define mv_instr_op2   mv_mepa_I[mv_mepa_i].op2
#define mv_instr_op3   mv_mepa_I[mv_mepa_i].op3
#define mv_instr_opR   mv_mepa_I[mv_mepa_i].endDesvio


// ------------------------------------------------------------------------
// Procedimentos
// ------------------------------------------------------------------------


void inicia_MV_mepa ();
void insereInstr_MV_mepa (char* rotulo, instrucao_mepa instr,
                          int op1, int op2, int op3, int op4,
                          char* rotDdesvio);

void relocaDesvios_MV_mepa();


// ------------------------------------------------------------------------
// Executa a instrução indicada pelo registrador "i" e retorna  o
// código de instrução executada.
// ------------------------------------------------------------------------

instrucao_mepa executa_instr_MV_mepa();






/* ===================== interface_usuario.h ===================== */
/* -------------------------------------------------------------------
 *            Arquivo: acao_usuario.h
 * -------------------------------------------------------------------
 *              Autor: Bruno Muller Junior
 *               Data: 09/2020
 *      Atualizado em: [20/09/2020, 07h:56m]
 *
 * -------------------------------------------------------------------
 *
 * ------------------------------------------------------------------- */
typedef struct params_linha_comando {
  FILE* fp_in;        // arquivo entrada
  char arq_in[100];   // nome arquivo entrada
  int interativo;     // executa passo a passo ou de uma vez
} params_linha_comando;

typedef enum cmd_usr {
  sair, imprime, resume, proximo
} cmd_usr;


void iniciaInterfaceUsuario();

/* -------------------------------------------------------------------
 * Os valores gerados por IMPR são armazenados no vetor abaixo e
 * são impressos logo antes da linha de interação com o usuário.  
 * ------------------------------------------------------------------- */
void insere_fim_vetor_impr(int elem);

/* -------------------------------------------------------------------
 * Os parâmetros incluídos na linha de invocação do programa são 
 * extraídos pelo procedimento abaixo.
 * ------------------------------------------------------------------- */
params_linha_comando* le_params_linha_comando (int argc, char** argv);
void imprime_params_linha_comando (params_linha_comando* olc);// util para depurar

/* -------------------------------------------------------------------
 * Leitura dá opção de interação do usuário solicitada após a impressão 
 * do estado da máquina virtual
 * ------------------------------------------------------------------- */
cmd_usr le_comando();

/* -------------------------------------------------------------------
 * TODO: Quando o programa MEPA é muito grande, ele não cabe na tela. 
 * Quero adaptar o procedimento abaixo para imprimir o vetor de instruções 
 * centralizado na instrução atual.
 * ------------------------------------------------------------------- */
void printf_interv_instr_MV_mepa (int delta, int atual);

/* -------------------------------------------------------------------
 * O termo "estado" significa os valores atuais de cada uma das 
 * estruturas da máquina virtual.
 * ------------------------------------------------------------------- */
void imprime_estado_MV_mepa_interativo();
void imprime_estado_MV_mepa_batch(); // só imprime o result de IMPR

/* ===================== executaMepa.h ===================== */
void executaMepa(params_linha_comando* olc);

/* ===================== MV_mepa.c ===================== */


/* -------------------------------------------------------------------
 *            Arquivo: MV_mepa.c
 * -------------------------------------------------------------------
 *              Autor: Bruno Muller Junior
 *               Data: 09/2020
 *      Atualizado em: [27/09/2020, 17h:49m]
 *
 * -------------------------------------------------------------------
 * Implementacoes dos procedimentos
 * para acesso as cinco estruturas da maquina virtual.
 * ------------------------------------------------------------------- */
MV_mepa mv_mepa;

void inicia_MV_mepa () {
  mv_mepa_tam_i = 0;
  mv_mepa_i = 0;
  iniciaInterfaceUsuario();
} 
 

/* -------------------------------------------------------------------
 *
 * ------------------------------------------------------------------- */

void insereInstr_MV_mepa (char* rotulo, instrucao_mepa instr,
                          int op1, int op2, int op3, int op4,
                          char* rotDdesvio) {
  instStruct* VI=&mv_mepa_I[mv_mepa_tam_i];

  strcpy(VI->rotulo,rotulo);
  strcpy(VI->rotDdesvio,rotDdesvio);
  VI->endDesvio = -1;
  
  VI->instr = instr;
  VI->op1 = op1;
  VI->op2 = op2;
  VI->op3 = op3;
  mv_mepa_tam_i = mv_mepa_tam_i+1;
}




/* -------------------------------------------------------------------
 *    relocacao dos enderecos de desvio
 * ------------------------------------------------------------------- */

typedef struct reloc{
  char* rotulo;
  int indice;
} reloc;

reloc tabReloc[1024];

void  relocaDesvios_MV_mepa(){
  int i, k, j, max_tab_reloc;

  // ------------------------------------------------------------------------------
  // obtem os enderecos dos rotulos  (nao testa duplicados)
  j=0;
  for (i=0; i<mv_mepa_tam_i; i++) {
    if (strlen(mv_mepa_I[i].rotulo) != 0) {
      tabReloc[j].rotulo=mv_mepa_I[i].rotulo;
      tabReloc[j].indice=i;
      j++;
    }
  }
  max_tab_reloc=j;
  // ------------------------------------------------------------------------------
  // coloca os enderecos-alvo dos desvios em endDesvio
  for (i=0; i<mv_mepa_tam_i; i++) {  // para cada instrução do Vetor de Instrucoes,
    if (strlen(mv_mepa_I[i].rotDdesvio) != 0) { // esta instr tem rotulo de desvio?
      for (j=0; j<max_tab_reloc; j++) { // procura pelo rotulo na tabela de desvios 
        if (strcmp (mv_mepa_I[i].rotDdesvio, tabReloc[j].rotulo)==0) {
          mv_mepa_I[i].endDesvio = tabReloc[j].indice; // achou: coloca end.
        }
      }
    }
  }
}








/* -------------------------------------------------------------------
 *    Executa uma instrucao
 * ------------------------------------------------------------------- */


instrucao_mepa _executa_instr_MV_mepa( int* i, int* s,
                              instStruct *I, int *D, int *M) {
  instrucao_mepa instr_exec = I[*i].instr;

  int k = mv_instr_op1;
  int m = mv_instr_op1;
  int n = mv_instr_op2;
  int j = mv_instr_op2;
  int p = mv_instr_opR;

  
  switch (instr_exec) {

//----------------* AMEM k
  case amem:
    *s = *s + k;
    *i = *i + 1;
    break;
    
//---------------- ARMI m,n
  case armi:
    M[M[D[m]+n]] = M[*s];
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* ARMZ m,n
  case armz:
    M[D[m]+n] = M[*s];
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CHPR p,n
  case chpr:
    M[*s+1] = *i+1;  
    M[*s+2] = n;  
    *s = *s + 2;
    *i = p;
    break;
//---------------- CMAG
  case cmag:
    M[*s-1] = (M[*s-1] >= M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CMDG
  case cmdg:
    M[*s-1] = (M[*s-1] != M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CMEG
  case cmeg:
    M[*s-1] = (M[*s-1] <= M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CMIG
  case cmig:
    M[*s-1] = (M[*s-1] == M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CMMA
  case cmma:
    M[*s-1] = (M[*s-1] > M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------- CMME
  case cmme:
    M[*s-1] = (M[*s-1] < M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* CONJ
  case conj:
    M[*s-1] = (M[*s-1] & M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* CRCT k
  case crct:
    *s = *s + 1;
    M[*s] = k;     
    *i = *i + 1;
    break;
//---------------- CREN m,n
  case cren:
    *s = *s + 1;
    M[*s] = D[m]+n;   
    *i = *i + 1;
    break;
//---------------- CRVI m,n
  case crvi:
    *s = *s + 1;
    M[*s] = M[M[D[m]+n]];
    *i = *i + 1;
    break;
//----------------* CRVL m,n
  case crvl:
    *s = *s + 1;
    M[*s] = M[D[m]+n];
    *i = *i + 1;
    break;
//----------------* DISJ
  case disj:
    M[*s-1] = (M[*s-1] | M[*s]) ? 1 : 0;  
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* DIVI
  case divi:
    M[*s-1] = (M[*s-1] / M[*s]);  
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* DMEM k
  case dmem:
    *s = *s - k;
    *i = *i + 1;
    break;
//---------------- DSVF p
  case dsvf:
    *i = (M[*s] == 0) ? p : *i+1;  
    *s = *s - 1;
    break;
//---------------- DSVR p,n,j
  case dsvr:
    {
      int atual = n;
      int dest = j;
      int temp;
      while (atual != dest) {
        temp = M[D[atual]-2];
        D[atual]=M[D[atual]-1];
        atual = temp;
      }
      *i = p;
      break;
    }
//---------------- DSVS p
  case dsvs:
    *i = p;
    break;
//---------------- ENPR m
  case enpr:
    *s = *s + 1;
    M[*s] = D[m];
    D[m] = *s+1;
    *i = *i + 1;
    break;
//---------------- ENRT m,n
  case enrt:
    *s = D[m] + n - 1;
    *i = *i + 1;
    break;
//---------------- IMPR
  case impr:
    insere_fim_vetor_impr(M[*s]);
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* INPP
  case inpp:
    *s = -1;
    D[0] = 0;
    *i = *i + 1;
    break;
//----------------* INVR
  case invr:
    M[*s] = (-1) * M[*s];  
    *i = *i + 1;
    break;
//---------------- LEIT
  case leit:
    *s = *s + 1;
    printf("(entrada):");
    scanf("%d", &M[*s]);
    getchar(); // consome o \n
    *i = *i + 1;
    break;
//----------------* MULT
  case mult:
    M[*s-1] = (M[*s-1] * M[*s]);  
    *s = *s - 1;
    *i = *i + 1;
    break;
//---------------*- NADA
  case nada:
    *i = *i + 1;
    break;
//----------------* NEGA
  case nega:
    M[*s] = 1 - M[*s];  
    *i = *i + 1;
    break;
//----------------* PARA
  case para:
  // como nao muda *i, fica preso aqui.
  break;
//---------------- RTPR m,n
  case rtpr:
    D[k]=M[*s];
    *i = M[*s-2];
    *s = *s-(n+3);
    break;
//----------------* SOMA
  case soma:
    M[*s-1] = (M[*s-1] + M[*s]);
    *s = *s - 1;
    *i = *i + 1;
    break;
//----------------* SUBT
  case subt:
    M[*s-1] = (M[*s-1] - M[*s]);  
    *s = *s - 1;
    *i = *i + 1;
    break;
  }

  //  imprimePilha(5, D, M, *s);
  
  return instr_exec;
}


instrucao_mepa executa_instr_MV_mepa() {

  return _executa_instr_MV_mepa(&mv_mepa_i,
                                &mv_mepa_s,
                                mv_mepa_I,
                                mv_mepa_D,
                                mv_mepa_M
                                );
  
}

/* ===================== interface_usuario.c ===================== */
/* -------------------------------------------------------------------
 *            Arquivo: interface_usuario.c
 * -------------------------------------------------------------------
 *              Autor: Bruno Muller Junior
 *               Data: 09/2020
 *      Atualizado em: [20/09/2020, 07h:54m]
 *
 * -------------------------------------------------------------------
 * Implementação de todos os procedimentos de leitura ou de impressão 
 * na tela.
 * ------------------------------------------------------------------- */
#define CLEAR_SCREEN "\e[1;1H\e[2J"

char* _01_resumo_param_linha_comando =
    "-----------------------------------------\n"
    " Arquivo de entrada: %s                  \n"
    " Modo Interativo:    %s                  \n"
    " Iniciando Execução                      \n"
    "-----------------------------------------\n";

char* _02_ajuda_linha_comando = 
  "uso: compilaMepa [-r -h -iArqIn]                   \n"
  "Onde:                                              \n"
  "    -r => desativa modo interativo (default=ativo) \n"
  "    -h => help                                     \n"
  "    -iARQ => arquivo de entrada (default=MEPA)     \n";

char* _03_ajuda_modo_interativo =
  "Modo interativo:                   \n"
  "       q => sai                    \n"
  "       r => resume execucao        \n"
  "       ? => imprime esta mensagem  \n"
  "  outros => proximo comando        \n";

/* -------------------------------------------------------------------
 *   Trata linha de comando
 * ------------------------------------------------------------------- */
void imprime_params_linha_comando (params_linha_comando* plc) {

  printf (_01_resumo_param_linha_comando,
          plc->arq_in,
          (plc->interativo==1) ? "sim" : "nao"); 

  if (plc->interativo) {
    printf("%s", _03_ajuda_modo_interativo);
  }
}



/* -------------------------------------------------------------------
 *  carrega  params_linha_comando
 * ------------------------------------------------------------------- */


params_linha_comando* le_params_linha_comando (int argc, char** argv) {
  int c;
  params_linha_comando *plc;

  plc = (params_linha_comando*) malloc (sizeof(params_linha_comando));
  plc->interativo = 1;
  strcpy(plc->arq_in, "");

  while ((c = getopt (argc, argv, "rphi:")) != -1) {
    switch (c)
      {
      case 'r': 
        plc->interativo = 0;
        break;
      case 'i':
        strcpy(plc->arq_in, optarg);
      break;
      default:
        printf("%s", _02_ajuda_linha_comando);
        exit(0);
        break;
      }
  }

  // Se não disse o nome do arquivo de entrada, 
  // então a entrada é o arquivo "MEPA"
  if (strlen(plc->arq_in)==0)
    strcpy (plc->arq_in, "MEPA");

  plc->fp_in=fopen (plc->arq_in, "r");
  if (plc->fp_in == NULL) {
    printf("Arquivo %s não encontrado\n", plc->arq_in);
    exit(-1);
  }
  return plc;
}



/* -------------------------------------------------------------------
 *  Trata modo interativo: le comando do usuário
 * ------------------------------------------------------------------- */
cmd_usr le_comando(){
  char linha[20];
  char cmd;
  int i=0;

  while (1) {
    printf("=> "); // prompt
    scanf("%c", &cmd);
    if (cmd == '\n')
      cmd = 's';
    else
      getchar(); // consome o '\n'
    
    switch (cmd) {
    case '?':
      printf("%s", _03_ajuda_modo_interativo);
      break;
    case 'q': return sair;
    case 'r': return resume;
    default : return proximo;
    }
  
  }
}

// *********************************************************************
// *********************************************************************
// Operacoes sobre o vetor_impr (armazena results do IMPR
// *********************************************************************

#define TAM_VET_IMPR 100
int _vetor_impr[TAM_VET_IMPR];
int _inicio_vetor_impr =0;
int _fim_vetor_impr =-1;
void insere_fim_vetor_impr(int elem){
  _fim_vetor_impr++;
  if (_fim_vetor_impr >= TAM_VET_IMPR) {
    printf ("Overflow do buffer de impressao\n");
    exit (-1);
  }
  _vetor_impr[_fim_vetor_impr] = elem;
}

void imprime_vetor_impr(char *s){
  s[0]='\0';
  for (int i=_inicio_vetor_impr; i<=_fim_vetor_impr; i++)
    sprintf(s+strlen(s), "%d ", _vetor_impr[i]);
}

int alguem_no_vetor_impr() {
  return(_inicio_vetor_impr == _fim_vetor_impr);
}

void anda_inicio_vetor_impr() {
  _inicio_vetor_impr++;
  return;
}


// *********************************************************************
// *********************************************************************
// Impressao da maquina virtual em execucao
// 1: Vetor de instrucoes
// *********************************************************************


 

/* -------------------------------------------------------------------
 *  Mapa entre códigos de operação e o string a ser impresso.
 * ------------------------------------------------------------------- */
char _tabInstr[100][100];
void iniciaTabInstr ()
{
  strcpy(_tabInstr[inpp], "INPP"); 
  strcpy(_tabInstr[para], "PARA"); 
  strcpy(_tabInstr[nada], "NADA"); 
  strcpy(_tabInstr[soma], "SOMA"); 
  strcpy(_tabInstr[subt], "SUBT"); 
  strcpy(_tabInstr[mult], "MULT"); 
  strcpy(_tabInstr[divi], "DIVI");
  strcpy(_tabInstr[invr], "INVR");
  strcpy(_tabInstr[conj], "CONJ");
  strcpy(_tabInstr[disj], "DISJ");
  strcpy(_tabInstr[nega], "NEGA"); 
  strcpy(_tabInstr[cmme], "CMME"); 
  strcpy(_tabInstr[cmma], "CMMA"); 
  strcpy(_tabInstr[cmig], "CMIG"); 
  strcpy(_tabInstr[cmdg], "CMDG"); 
  strcpy(_tabInstr[cmeg], "CMEG"); 
  strcpy(_tabInstr[cmag], "CMAG"); 
  strcpy(_tabInstr[leit], "LEIT"); 
  strcpy(_tabInstr[impr], "IMPR"); 
  strcpy(_tabInstr[crct], "CRCT"); 
  strcpy(_tabInstr[amem], "AMEM"); 
  strcpy(_tabInstr[dmem], "DMEM"); 
  strcpy(_tabInstr[enpr], "ENPR"); 
  strcpy(_tabInstr[dsvs], "DSVS"); 
  strcpy(_tabInstr[dsvf], "DSVF"); 
  strcpy(_tabInstr[crvl], "CRVL"); 
  strcpy(_tabInstr[armz], "ARMZ"); 
  strcpy(_tabInstr[crvi], "CRVI");
  strcpy(_tabInstr[crvi], "CRVI"); 
  strcpy(_tabInstr[armi], "ARMI"); 
  strcpy(_tabInstr[cren], "CREN"); 
  strcpy(_tabInstr[enrt], "ENRT"); 
  strcpy(_tabInstr[chpr], "CHPR"); 
  strcpy(_tabInstr[rtpr], "RTPR"); 
  strcpy(_tabInstr[dsvr], "DSVR"); 
}

/* -------------------------------------------------------------------
 * Coloca em "sout" a impressão da instrução contida em mv_mepa_I[i].
 * O parâmetro "primeiro" é incluído no primeiro caractere de "sout".
 * ------------------------------------------------------------------- */

void gera_linha_impr_Instr_MV_mepa (char* sout, int i, char *primeiro) {

  sprintf(sout, "%s", "");

  // Se estiver fora do intervalo de instrucoes, retorna linha vazia
  if (i<0 || i>mv_mepa_tam_i-1)
    return;

  // concatena primeiro
  sprintf(sout+strlen(sout), "%s", primeiro);
  
  // concatena endereco da instrucao
  sprintf (sout+strlen(sout),"I[%04d] ", i);
    
  // concatena nome do rotulo (se houver)
  if (strlen(mv_mepa_I[i].rotulo) == 0) 
    sprintf (sout+strlen(sout),"     ");
  else
    sprintf (sout+strlen(sout),"%s: ", mv_mepa_I[i].rotulo);

  // concatena nome da instrucao
  sprintf (sout+strlen(sout),"%s ", _tabInstr[mv_mepa_I[i].instr]);

  // concatena parametros de acordo com a instrucao
  switch (mv_mepa_I[i].instr) {
    // caso 1: instrucao com zero parametros
  case inpp: case para: case soma: case subt: case mult: case divi:
  case invr: case conj: case disj: case nega: case cmme: case cmma:
  case cmig: case cmdg: case cmeg: case cmag: case nada: case leit:
  case impr:
    break;
    // ------------------------------------------------------------
    // caso 2: instrucao com um parametro: inteiro
  case crct: case amem: case dmem: case enpr: 
    sprintf (sout+strlen(sout),"%d", mv_mepa_I[i].op1);
    break;
    // ------------------------------------------------------------
    // caso 3: instrucao com um parametro: rotulo
  case dsvs: case dsvf:
    sprintf (sout+strlen(sout),"%s", mv_mepa_I[i].rotDdesvio);
    break;
    // ------------------------------------------------------------
    // caso 4: instrucao com dois parametros: dois inteiros
  case crvl: case armz: case crvi: case armi: case cren: case enrt:
    sprintf (sout+strlen(sout),"%d, %d", mv_mepa_I[i].op1,
             mv_mepa_I[i].op2);
    break;
    // ------------------------------------------------------------
    // caso 5: instrucao com dois parametros: um rotulo, um inteiro
  case chpr: 
    sprintf (sout+strlen(sout),"%s, %d", mv_mepa_I[i].rotDdesvio,
             mv_mepa_I[i].op2);
    break;
    // ------------------------------------------------------------
    // caso 6: instrucao com tres parametros: tres inteiros
  case dsvr: 
    sprintf (sout+strlen(sout),"%s, %d, %d", mv_mepa_I[i].rotDdesvio,
             mv_mepa_I[i].op2, mv_mepa_I[i].op3);
    break;
  }
}

/* -------------------------------------------------------------------
 * Imprime todo o vetor de instruções sem fazer nenhum refinamento. 
 * Foi muito utilizada na fase de desenvolvimento e testes, mas talvez 
 * nao tenha mais nenhuma utilidade.
 * ------------------------------------------------------------------- */

void dump_todas_instr_MV_mepa () {
  char sout[100];
  for (int i=0; i<mv_mepa_tam_i-1; i++) {
    gera_linha_impr_Instr_MV_mepa(sout, i," ");
    printf("%s\n", sout);
  }
}

// *********************************************************************
// *********************************************************************
// Impressao da maquina virtual em execucao
// 2: Vetor de regs de base
// *********************************************************************



void gera_linha_impr_base_MV_mepa (char* sout, int* D, int l){
  sprintf(sout, "%s", "");

  if (l<0 || l>4)
    return;

  sprintf (sout+strlen(sout),"D[%04d] %04d", l, D[l]);

}

// *********************************************************************
// *********************************************************************
// Impressao da maquina virtual em execucao
// 3: Vetor da pilha
// *********************************************************************

void gera_linha_impr_pilha_MV_mepa (char* sout, int* M, int l){
  sprintf(sout, "%s", "");

  if (l<0 || l>mv_mepa_s)
    return;

  sprintf (sout+strlen(sout),"M[%04d] %04d", l, M[l]);

  if (l == mv_mepa_s)
    sprintf (sout+strlen(sout), "<");
  else
    sprintf (sout+strlen(sout), " ");
}



/* -------------------------------------------------------------------
 * O procedimento abaixo anexa espaços em branco até o índice indicado 
 * (pos). O vetor brancos só foi criado para agilizar o processo.
 * ------------------------------------------------------------------- */

char *_brancos="                                        ";
void  preenche_ate_posicao(char *s, int pos) {
  int tam = strlen(s);
  strncpy(&s[tam], _brancos, pos-tam);
  s[pos]='\0';
}  

/* -------------------------------------------------------------------
 * Apresenta "graficamente" a situacao atual (estado) da máquina 
 * virtual mepa.
 * ------------------------------------------------------------------- */

void imprime_estado_MV_mepa_interativo() {
  char linha_I[100];
  char linha_M[100];
  char linha_D[100];
  char linha_out[100];
  char linha_impr[TAM_VET_IMPR];

  // Imprime todas as instruções na primeira coluna e, baseado na
  // altura, colunas com o vetor de registradores de base e a pilha.
  // Observe que os índices dos vetores de instrução são crescentes
  // (do começo do vetor, índice zero até o fim do vetor,
  // mv_mepa_tam_i) enquanto que os vetores de registrador de base e
  // da pilha são decrescentes. Para ajustá-los a a base da figura, é
  // utilizada a fórmula (mv_mepa_tam_i-j-1);

  printf(CLEAR_SCREEN);
  for(int j=0; j<mv_mepa_tam_i; j++) {
    if (j==mv_mepa_i)
      gera_linha_impr_Instr_MV_mepa(linha_I, j, ">");//indica instr a ser exec
    else
      gera_linha_impr_Instr_MV_mepa(linha_I, j, " ");
    preenche_ate_posicao(linha_I, 40);

    gera_linha_impr_pilha_MV_mepa(linha_M, mv_mepa_M, mv_mepa_tam_i-j-1);
    preenche_ate_posicao(linha_M, 20); 

    gera_linha_impr_base_MV_mepa (linha_D, mv_mepa_D, mv_mepa_tam_i-j-1);
    preenche_ate_posicao(linha_D,20);
    
    printf ("%s %s %s \n", linha_I, linha_D, linha_M);
  }
  imprime_vetor_impr(linha_impr);
  printf ("saida: %s \n", linha_impr);
}


/* -------------------------------------------------------------------
 * em modo de execução batch (!interativo), só imprime o resultado de IMPR
 * ------------------------------------------------------------------- */

void imprime_estado_MV_mepa_batch() {
  char linha_impr[TAM_VET_IMPR];

  if (alguem_no_vetor_impr()) {
      imprime_vetor_impr(linha_impr);
      printf ("saida: %s \n", linha_impr);
      anda_inicio_vetor_impr();
  }
}



void iniciaInterfaceUsuario() {
  iniciaTabInstr ();
}

/* ===================== executaMepa.c ===================== */
/* -------------------------------------------------------------------
 *            Arquivo: executaMepa.c
 * -------------------------------------------------------------------
 *              Autor: Bruno Muller Junior
 *               Data: 09/2020
 *      Atualizado em: [20/09/2020, 08h:05m]
 *
 * -------------------------------------------------------------------
 * Talvez o melhor lugar de moradia deste procedimento seja no modulo
 * de interface com o usuario. Achei que ficava melhor em um arquivo
 * separado , e por isso ficou aqui.
 * ------------------------------------------------------------------- */

/* -------------------------------------------------------------------
 * Laço de execucaoo de comandos MEPA. Ja entra executando o primeiro 
 * comando do arquivo, normalmente INPP.
 * ------------------------------------------------------------------- */
/*
TODO (MEPA): Revisar a execucao sem modificar as instrucoes.
  1. Inicializar cmd antes do laco, inclusive no modo -r.
  2. Manter o modo interativo e o modo nao interativo da professora.
  3. Preservar os valores inteiros lidos por LEIT e impressos por IMPR.
*/
void executaMepa(params_linha_comando* plc) {
  cmd_usr cmd;
  instrucao_mepa instr_exec;
  
  do {
    instr_exec = executa_instr_MV_mepa();
    if (plc->interativo)
      imprime_estado_MV_mepa_interativo();
    else
      imprime_estado_MV_mepa_batch();

    if (instr_exec == para) {
      printf("execução finalizada\n");
      break;
    }

    if (plc->interativo) {
      cmd = le_comando();
      if (cmd == resume)
        plc->interativo = 0;
    }
  }  while (cmd != sair);
}

/* =====================================================================
 * LEITOR (substitui leMepa.l + leMepa.y)
 *
 * Gramatica original:   linha : [ROTULO ':'] COMANDO [P1 [','] [P2 [','] [P3]]]
 *   ROTULO  = [RL][0-9]+        (no maximo 9 caracteres: o campo tem 10 bytes)
 *   NUMERO  = [-+]?[0-9]+
 *   a virgula entre parametros e opcional
 *
 *   sem parametro : INPP PARA SOMA SUBT MULT DIVI INVR CONJ DISJ NEGA CMME
 *                   CMMA CMIG CMDG CMEG CMAG NADA LEIT IMPR
 *   1 inteiro     : CRCT AMEM DMEM ENPR
 *   1 rotulo      : DSVS DSVF
 *   2 inteiros    : CRVL ARMZ CRVI ARMI CREN ENRT RTPR
 *   rotulo, int   : CHPR
 *   rotulo, 2 int : DSVR
 *
 * Cada linha vira uma chamada igual a do leMepa.y:
 *   insereInstr_MV_mepa(rotulo, instr, op1, op2, op3, 0, rotuloDesvio);
 * (sem parametros: ops 0; com um rotulo: op1 = 0 e rotuloDesvio = o rotulo;
 *  CHPR: op2 = n; DSVR: op2 e op3.)
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * LEITOR - formato aceito (um comando por linha):
 *
 *     [ROTULO ':'] COMANDO [P1 [','] [P2 [','] [P3]]]
 *
 *   ROTULO = [RL][0-9]+ (no maximo 9 caracteres: o campo tem 10 bytes)
 *   NUMERO = [-+]?[0-9]+  (a virgula entre parametros e opcional)
 *
 * EXTENSAO PARA O COMPILADOR (o enunciado da Fase 2 pede variaveis
 * referenciadas pelo NOME no TXT):
 *   - linhas "; VAR nome endereco" (geradas pelo compilador.c) declaram
 *     o endereco de cada nome na memoria da MEPA. Qualquer outra linha
 *     que comece com ';' e comentario e e ignorada.
 *   - nas instrucoes CRVL, ARMZ, CRVI, ARMI e CREN o operando pode ser
 *     "m,n" (formato original) ou um NOME. Um nome vira "0,endereco"
 *     (nivel lexico 0, deslocamento = endereco declarado em "; VAR").
 *
 * O leitor trabalha em duas passadas sobre o arquivo: a primeira so
 * coleta as linhas "; VAR"; a segunda monta as instrucoes e chama
 * insereInstr_MV_mepa(), na ordem em que aparecem. As 34 instrucoes da
 * maquina nao foram alteradas.
 * --------------------------------------------------------------------- */

#define LEITOR_MAX_LINHA  256    /* tamanho maximo de uma linha do TXT    */
#define LEITOR_MAX_VARS   1100   /* variaveis + $ret_ + temporarios       */
#define LEITOR_TAM_NOME   80     /* nome/token (inclui o '\0')            */
#define LEITOR_TAM_ROT    10     /* igual ao campo rotulo[10] da instrucao */

/* quantos e quais operandos cada instrucao recebe */
typedef enum formato_instr {
  FMT_NENHUM,   /* sem operandos                                  */
  FMT_INT1,     /* um inteiro                                     */
  FMT_ROTULO,   /* um rotulo de desvio                            */
  FMT_INT2,     /* dois inteiros                                  */
  FMT_VAR2,     /* "m,n" ou NOME de variavel (vira 0,endereco)    */
  FMT_CHPR,     /* rotulo e um inteiro                            */
  FMT_DSVR      /* rotulo e dois inteiros                         */
} formato_instr;

typedef struct entrada_mnemonico {
  const char *nome;
  instrucao_mepa instr;
  formato_instr formato;
} entrada_mnemonico;

/* TODO[M-01]: as 34 instrucoes da MEPA */
static const entrada_mnemonico tabMnemonicos[] = {
  {"INPP", inpp, FMT_NENHUM}, {"PARA", para, FMT_NENHUM},
  {"SOMA", soma, FMT_NENHUM}, {"SUBT", subt, FMT_NENHUM},
  {"MULT", mult, FMT_NENHUM}, {"DIVI", divi, FMT_NENHUM},
  {"INVR", invr, FMT_NENHUM}, {"CONJ", conj, FMT_NENHUM},
  {"DISJ", disj, FMT_NENHUM}, {"NEGA", nega, FMT_NENHUM},
  {"CMME", cmme, FMT_NENHUM}, {"CMMA", cmma, FMT_NENHUM},
  {"CMIG", cmig, FMT_NENHUM}, {"CMDG", cmdg, FMT_NENHUM},
  {"CMEG", cmeg, FMT_NENHUM}, {"CMAG", cmag, FMT_NENHUM},
  {"NADA", nada, FMT_NENHUM}, {"LEIT", leit, FMT_NENHUM},
  {"IMPR", impr, FMT_NENHUM},
  {"CRCT", crct, FMT_INT1},   {"AMEM", amem, FMT_INT1},
  {"DMEM", dmem, FMT_INT1},   {"ENPR", enpr, FMT_INT1},
  {"DSVS", dsvs, FMT_ROTULO}, {"DSVF", dsvf, FMT_ROTULO},
  {"CRVL", crvl, FMT_VAR2},   {"ARMZ", armz, FMT_VAR2},
  {"CRVI", crvi, FMT_VAR2},   {"ARMI", armi, FMT_VAR2},
  {"CREN", cren, FMT_VAR2},
  {"ENRT", enrt, FMT_INT2},   {"RTPR", rtpr, FMT_INT2},
  {"CHPR", chpr, FMT_CHPR},   {"DSVR", dsvr, FMT_DSVR}
};
static const int totalMnemonicos =
    (int) (sizeof(tabMnemonicos) / sizeof(tabMnemonicos[0]));

/* nomes declarados pelas linhas "; VAR nome endereco" */
typedef struct entrada_var {
  char nome[LEITOR_TAM_NOME];
  int endereco;
} entrada_var;

static entrada_var tabVarsLeitor[LEITOR_MAX_VARS];
static int nVarsLeitor = 0;

/* TODO[M-04]: linha invalida -> mensagem com o numero da linha e exit(-1) */
static void erroLeitor(int linha, const char *motivo, const char *detalhe) {
  printf("Erro no arquivo MEPA, linha %d: %s", linha, motivo);
  if (detalhe != NULL && detalhe[0] != '\0') printf(" '%s'", detalhe);
  printf("\n");
  exit(-1);
}

static void pulaBrancos(const char **p) {
  while (**p == ' ' || **p == '\t') (*p)++;
}

static void pulaSeparadores(const char **p) {
  while (**p == ' ' || **p == '\t' || **p == ',') (*p)++;
}

/* 1 se na linha so resta espaco, quebra de linha ou comentario */
static int fimDeLinha(const char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  return (*p == '\0' || *p == ';');
}

/* le letras, digitos e '_' (usado para rotulo e mnemonico). devolve o
   tamanho lido; se passar de tam-1 o chamador trata como erro */
static int leIdentificador(const char **p, char *buf, int tam) {
  int n = 0;
  while (isalnum((unsigned char) **p) || **p == '_') {
    if (n < tam - 1) buf[n] = **p;
    n++;
    (*p)++;
  }
  buf[n < tam - 1 ? n : tam - 1] = '\0';
  return n;
}

/* le um operando (ate branco, virgula, ';' ou fim de linha). devolve o
   tamanho; o texto e truncado em tam-1 */
static int leToken(const char **p, char *buf, int tam) {
  int n = 0;
  pulaSeparadores(p);
  while (**p != '\0' && **p != ' ' && **p != '\t' && **p != ',' &&
         **p != ';' && **p != '\r' && **p != '\n') {
    if (n < tam - 1) buf[n] = **p;
    n++;
    (*p)++;
  }
  buf[n < tam - 1 ? n : tam - 1] = '\0';
  return n;
}

/* leToken com os erros comuns: operando ausente ou grande demais */
static void leOperando(const char **p, char *buf, int linha) {
  int n = leToken(p, buf, LEITOR_TAM_NOME);
  if (n == 0) erroLeitor(linha, "operando ausente", "");
  if (n >= LEITOR_TAM_NOME) erroLeitor(linha, "operando grande demais", buf);
}

/* TODO[M-05]: rotulos de ate 9 caracteres, no formato [RL][0-9]+ */
static int rotuloValido(const char *s) {
  int i;
  int n = (int) strlen(s);
  if (n < 2 || n > LEITOR_TAM_ROT - 1) return 0;
  if (s[0] != 'L' && s[0] != 'R') return 0;
  for (i = 1; i < n; i++) {
    if (!isdigit((unsigned char) s[i])) return 0;
  }
  return 1;
}

static int numeroValido(const char *s) {
  int i = 0;
  if (s[0] == '+' || s[0] == '-') i = 1;
  if (s[i] == '\0') return 0;
  for (; s[i] != '\0'; i++) {
    if (!isdigit((unsigned char) s[i])) return 0;
  }
  return 1;
}

static int converteInteiro(const char *tok, int linha) {
  long long v;
  if (strlen(tok) > 12) erroLeitor(linha, "numero grande demais", tok);
  v = strtoll(tok, NULL, 10);
  if (v > 2147483647LL || v < -2147483648LL) erroLeitor(linha, "numero grande demais", tok);
  return (int) v;
}

static int leInteiro(const char **p, int linha) {
  char tok[LEITOR_TAM_NOME];
  leOperando(p, tok, linha);
  if (!numeroValido(tok)) erroLeitor(linha, "esperava um numero inteiro, veio", tok);
  return converteInteiro(tok, linha);
}

static void leRotuloOperando(const char **p, int linha, char *rotulo) {
  char tok[LEITOR_TAM_NOME];
  leOperando(p, tok, linha);
  if (!rotuloValido(tok)) erroLeitor(linha, "rotulo invalido", tok);
  strcpy(rotulo, tok);
}

static int buscaVarLeitor(const char *nome) {
  int i;
  for (i = 0; i < nVarsLeitor; i++) {
    if (strcmp(tabVarsLeitor[i].nome, nome) == 0) return tabVarsLeitor[i].endereco;
  }
  return -1;
}

/* PASSADA 1: registra a linha "; VAR nome endereco" (se for uma) */
static void registraVariavel(const char *linhaTxt, int nLinha) {
  const char *p = linhaTxt;
  char palavra[LEITOR_TAM_NOME];
  char nome[LEITOR_TAM_NOME];
  char num[LEITOR_TAM_NOME];
  int n, endereco;

  pulaBrancos(&p);
  if (*p != ';') return;
  p++;
  pulaBrancos(&p);
  leToken(&p, palavra, LEITOR_TAM_NOME);
  if (strcmp(palavra, "VAR") != 0) return;   /* comentario comum */

  n = leToken(&p, nome, LEITOR_TAM_NOME);
  if (n == 0) erroLeitor(nLinha, "VAR sem nome de variavel", "");
  if (n >= LEITOR_TAM_NOME) erroLeitor(nLinha, "nome de variavel grande demais", nome);
  n = leToken(&p, num, LEITOR_TAM_NOME);
  if (n == 0 || n >= LEITOR_TAM_NOME || !numeroValido(num) || num[0] == '-') {
    erroLeitor(nLinha, "VAR sem endereco valido para", nome);
  }
  endereco = converteInteiro(num, nLinha);
  if (endereco >= TAM_M) erroLeitor(nLinha, "endereco de variavel fora da memoria da MEPA", nome);
  if (buscaVarLeitor(nome) >= 0) erroLeitor(nLinha, "variavel declarada mais de uma vez", nome);
  if (nVarsLeitor >= LEITOR_MAX_VARS) erroLeitor(nLinha, "variaveis demais no arquivo MEPA", nome);

  strcpy(tabVarsLeitor[nVarsLeitor].nome, nome);
  tabVarsLeitor[nVarsLeitor].endereco = endereco;
  nVarsLeitor++;
}

/* PASSADA 2: interpreta uma linha e insere a instrucao na maquina */
static void processaLinha(const char *linhaTxt, int nLinha) {
  const char *p = linhaTxt;
  const char *q;
  char rotulo[LEITOR_TAM_ROT];
  char rotDesvio[LEITOR_TAM_ROT];
  char mnem[LEITOR_TAM_NOME];
  char tok[LEITOR_TAM_NOME];
  const entrada_mnemonico *e = NULL;
  int n, i, endereco;
  int op1 = 0, op2 = 0, op3 = 0;

  rotulo[0] = '\0';
  rotDesvio[0] = '\0';

  pulaBrancos(&p);
  if (*p == '\0' || *p == '\n' || *p == '\r' || *p == ';') return;  /* vazia/comentario */

  /* primeira palavra: e rotulo se vier seguida de ':', senao e o mnemonico */
  n = leIdentificador(&p, tok, LEITOR_TAM_NOME);
  if (n == 0) erroLeitor(nLinha, "esperava rotulo ou instrucao", "");
  if (n >= LEITOR_TAM_NOME) erroLeitor(nLinha, "palavra grande demais", tok);
  q = p;
  pulaBrancos(&q);
  if (*q == ':') {
    if (!rotuloValido(tok)) erroLeitor(nLinha, "rotulo invalido", tok);
    strcpy(rotulo, tok);
    p = q + 1;
    pulaBrancos(&p);
    n = leIdentificador(&p, mnem, LEITOR_TAM_NOME);
    if (n == 0) erroLeitor(nLinha, "falta a instrucao depois do rotulo", rotulo);
    if (n >= LEITOR_TAM_NOME) erroLeitor(nLinha, "palavra grande demais", mnem);
  } else {
    strcpy(mnem, tok);
  }

  for (i = 0; mnem[i] != '\0'; i++) mnem[i] = (char) toupper((unsigned char) mnem[i]);
  for (i = 0; i < totalMnemonicos; i++) {
    if (strcmp(tabMnemonicos[i].nome, mnem) == 0) {
      e = &tabMnemonicos[i];
      break;
    }
  }
  if (e == NULL) erroLeitor(nLinha, "instrucao desconhecida", mnem);

  switch (e->formato) {
    case FMT_NENHUM:
      break;
    case FMT_INT1:
      op1 = leInteiro(&p, nLinha);
      break;
    case FMT_ROTULO:
      leRotuloOperando(&p, nLinha, rotDesvio);
      break;
    case FMT_INT2:
      op1 = leInteiro(&p, nLinha);
      op2 = leInteiro(&p, nLinha);
      break;
    case FMT_VAR2:
      leOperando(&p, tok, nLinha);
      if (numeroValido(tok)) {            /* formato original: m,n */
        op1 = converteInteiro(tok, nLinha);
        op2 = leInteiro(&p, nLinha);
      } else {                            /* nome -> nivel 0, endereco */
        endereco = buscaVarLeitor(tok);
        if (endereco < 0) erroLeitor(nLinha, "variavel sem linha '; VAR nome endereco'", tok);
        op1 = 0;
        op2 = endereco;
      }
      break;
    case FMT_CHPR:
      leRotuloOperando(&p, nLinha, rotDesvio);
      op2 = leInteiro(&p, nLinha);
      break;
    case FMT_DSVR:
      leRotuloOperando(&p, nLinha, rotDesvio);
      op2 = leInteiro(&p, nLinha);
      op3 = leInteiro(&p, nLinha);
      break;
  }
  if (!fimDeLinha(p)) erroLeitor(nLinha, "operandos em excesso na instrucao", mnem);

  /* TODO[M-05]: respeitar TAM_I (4096) instrucoes */
  if (mv_mepa_tam_i >= TAM_I) erroLeitor(nLinha, "programa passa do limite de 4096 instrucoes", "");
  if (rotulo[0] != '\0') {
    for (i = 0; i < mv_mepa_tam_i; i++) {
      if (strcmp(mv_mepa_I[i].rotulo, rotulo) == 0) erroLeitor(nLinha, "rotulo definido mais de uma vez", rotulo);
    }
  }
  insereInstr_MV_mepa(rotulo, e->instr, op1, op2, op3, 0, rotDesvio);
}

/* todo desvio precisa apontar para um rotulo existente (a relocacao da
   professora nao testa isso e um destino ausente derrubaria a execucao) */
static void verificaDesvios(void) {
  int i, j, achou;
  for (i = 0; i < mv_mepa_tam_i; i++) {
    if (mv_mepa_I[i].rotDdesvio[0] == '\0') continue;
    achou = 0;
    for (j = 0; j < mv_mepa_tam_i; j++) {
      if (strcmp(mv_mepa_I[j].rotulo, mv_mepa_I[i].rotDdesvio) == 0) {
        achou = 1;
        break;
      }
    }
    if (!achou) {
      printf("Erro no arquivo MEPA: rotulo '%s' usado na instrucao %d nao foi definido\n",
             mv_mepa_I[i].rotDdesvio, i + 1);
      exit(-1);
    }
  }
}

/* le uma linha inteira do arquivo; devolve 0 no fim do arquivo */
static int leLinhaArquivo(FILE *fp, char *linhaTxt, int nLinha) {
  size_t tam;
  if (fgets(linhaTxt, LEITOR_MAX_LINHA, fp) == NULL) return 0;
  tam = strlen(linhaTxt);
  if (tam == (size_t) (LEITOR_MAX_LINHA - 1) && linhaTxt[tam - 1] != '\n') {
    erroLeitor(nLinha, "linha grande demais", "");
  }
  return 1;
}

/* le o arquivo plc->fp_in e carrega as instrucoes na maquina virtual */
static void leComandosMepa(params_linha_comando *plc) {
  char linhaTxt[LEITOR_MAX_LINHA];
  int nLinha;

  /* passada 1: so as linhas "; VAR nome endereco" */
  nLinha = 0;
  while (leLinhaArquivo(plc->fp_in, linhaTxt, nLinha + 1)) {
    nLinha++;
    registraVariavel(linhaTxt, nLinha);
  }

  /* passada 2: as instrucoes, na ordem do arquivo */
  rewind(plc->fp_in);
  nLinha = 0;
  while (leLinhaArquivo(plc->fp_in, linhaTxt, nLinha + 1)) {
    nLinha++;
    processaLinha(linhaTxt, nLinha);
  }
  fclose(plc->fp_in);

  if (mv_mepa_tam_i == 0) {
    printf("Erro no arquivo MEPA: nenhuma instrucao encontrada em %s\n", plc->arq_in);
    exit(-1);
  }
  verificaDesvios();
}

/* -------------------------------------------------------------------
 *  Programa Principal (igual ao de leMepa.y)
 * ------------------------------------------------------------------- */
/*
TODO (MEPA): Testes finais.
  1. Conferir os exemplos MEPA5, MEPA7.1 e MEPA7.2.
  2. Testar desvios, leitura, vetores e chamadas de rotina.
  3. Compilar com gcc -g -Og -Wall mepa.c -o mepa.
*/
int main (int argc, char** argv) {
  params_linha_comando *plc;

  plc = le_params_linha_comando(argc, argv);
  imprime_params_linha_comando (plc);

  inicia_MV_mepa ();

  // le programa indicado e o coloca na mv_mepa
  leComandosMepa(plc);
  relocaDesvios_MV_mepa();

  executaMepa(plc);
  return 0;
}