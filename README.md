# Projeto Compiladores
Alunos:
- Mateus Ribeiro Cerqueira - 10443901
- Pedro Henrique Carvalho Pereira - 10418861

## Execução

A compilação foi feita a partir da linha do enunciado do descritivo passado para o projeto:

```bash
gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
```

- `-Wall`: ativa os warnings do compilador
- `-Wno-unused-result`: desliga o aviso de resultado de função não utilizado (usado para `fread` de memoria, por exemplo).
- `-g`: permite debugar com `gdb`.
- `-Og`: usado para otimização.

Para executar:

```bash
./compilador <arquivo_fonte>
```

O compilador imprime os tokens e salva em `tokens_saida.txt`.

## Analisador Léxico

O analisador lex foi implementado pensando no `obterToken()`, que faz a leitura o código fonte inteiro para um buffer em memória e vai passanodo pelos caracteres por ponteiro (`bufPos`), usando as funções auxiliares  de `charAtual`, `proximoChar`, `avancarChar`. 

A função `obterToken()`:
1. Pula espaços em branco e comentários através de `pularEspacosEComentarios()`.
2. Olha o caractere atual e decide, a tipo de lexema: identificador/palavra reservada, inteiro/real (usando `.` para diferenciar `TOKEN_NUM_INT` de `TOKEN_NUM_FLOAT`), string, operadores relacionais e atribuição (`<`, `<=`, `<>`, `<-`, `>`, `>=`, `=`), aritméticos, intervalo de vetor (`..`) e os outros delimitadores (`( ) [ ] , :`).
3. Caso nenhum padrão seja reconhecido, a sequência é acumulada até o próximo espaço e chama o erro através do `erroLexico()` e depois encerra

Identificadores, strings e a maior parte dos operadores/delimitadores são armazenados em uma **tabela de símbolos** (`symtab` que é um vetor de strings) e o token guarda apenas o index (`attribute.table_index`), isso usando a estrutura de união de atributos definida na `Token` já definida no descritivo. As palavras reservadas estão em uma lista estática (`palavrasReservadas[]`) e identificadas pela função `ehPalavraReservada()`, identificadores por `findOrAddIdentifier()`. jà os operadores relacionais usam `OpRelAtributo` em vez da tabela de símbolos, já que são um conjunto pequeno.

Nesse contexto, a função `infoToken()` imprime e grava cada token como `linha# CATEGORIA | lexema`, tanto na tela quanto em `tokens_saida.txt`.

## Analisador Sintático

O analisador sintático foi implementado utilizando o algorítimo de **análise sintática descendente recursivo preditivo** seguindo a gramática definida pelo descritivo do projeto. Para isso, os não-terminais da gramática foram representados por funções no código, como `analisar_programa()` e `analisar_declaracoes()`.

A função `nextToken()`:
1. chama `obterToken()` para obter o próximo token do analisador léxico e armazena esse token em `lookahead`.
2. chama `infoToken()` para registrar a leitura do token.

A função `consome()`foi utilizada para verificar os terminais esperados pela gramática, ela:
1. recebe como parâmetro o tipo de token esperado e, quando necessário, recebe também o lexema esperado.
2. Caso o `lookahead` corresponda ao esperado, `nextToken()` é chamado e a análise continua.
3. Caso contrário, é chamado `erroSintatico()`, que informa a linha e o token incorreto e encerra a execução.

A análise começa em `analisar_programa()`, representando a estrutura geral de um programa na linguagem apresentada. A partir dela são analisadas as declarações de sub-rotinas, por `analisar_subrotinas()`, e variáveis, por `analisar_declaracoes()`, seguidas pelo bloco principal delimitado por `inicio` e `fimalgoritmo` e processado por `analisar_bloco_comandos()`. O bloco principal, chama repetidamente o método `analisar_comando()` até encontrar um terminal de fechamento de bloco. Então `analisar_comando()` identifica qual construção deve ser analisada.

As expressões foram divididas em diferentes níveis para representar a precedência dos operadores. `analisar_expressao()` inicia pela expressão lógica `analisar_expr_logica()`, seguida pela relacional `analisar_expr_relacional()`, aritmética `analisar_expr_aritmetica()`, termo `analisar_termo()` e fator `analisar_fator()`. Dessa forma, operadores `E` e `OU` são tratados em `analisar_expr_logica()`, operadores relacionais em `analisar_expr_relacional()`, `+` e `-` em `analisar_expr_aritmetica()` e `*`, `/`, `\` e `MOD` em `analisar_termo()`. Por fim, `analisar_fator()` reconhece valores inteiros, reais, strings, valores lógicos, identificadores, vetores, chamadas de função, expressões entre parênteses e operadores unários `+` e `-`.

Procedimentos e funções são reconhecidos por `analisar_subrotinas()`. Suas declarações são processadas respectivamente por `analisar_declaracao_procedimento()` e `analisar_declaracao_funcao()`, enquanto `analisar_lista_parametros()` trata os parâmetros e seus tipos.

Por fim ao encontrar uma construção que não corresponde à gramática esperada, `erroSintatico()` imprime uma mensagem de erro com a linha e o token que provocou o erro, e encerra o processo.

## Relatório de Design e Implementação

O repositório está organizado em:

- `compilador.c`: todo o código do compilador (lexico + sintático + main).
- `Makefile`: compila (`make build`), execução uma bateria de testes (`make test`), grava os resultados em `resultado_testes.txt`.
- `etapa1_gramatica.txt`: Etapa 1 para servir de base na implementação (define as expressões regulares e a gramática da linguagem MiniVisualg)
- `testes/`: casos de teste (`teste1.txt` a `teste16.txt`) com exemplos válidos e os exemplos 15 e 16 que geram erro léxico/sintático.
- `tokens_saida.txt` e `resultado_testes.txt`: saídas geradas.

Em suma , a solução foi pensada em cima da linha de compilação do descritivo, com estrutura: `obterToken()` / `infoToken()` / `nextToken()`, com os analisadores lexico e sintático dentro do arquivo `compilador.c` e comunicando por chamadas de função e variável global `lookahead`.

Outra solução possível, que chegamos a analisar, era separando o analisador léxico e o analisador sintático em dois códigos (por exemplo, `lexico.c` com seu `lexico.h` e `sintatico.c` com seu `sintatico.h`), definindo um `.h` compartilhado apenas com os protótipos de `obterToken()` e `infoToken()` e os tipos `Token` e `TokenNome`.

## Relatório de Uso de IA

Usamos o Claude Sonnet 5 em Baixo com pensamento estendido e o ChatGPT em dois momentos:
- Para ajudar com a arquitetura inicial do projeto
- Para gerar os casos de teste (`testes/teste1.txt` a `teste16.txt`) a partir da linguagem exemplificadas no Anexo I.

## Relatório de Bugs e Erros

Não encontramos bugs no compilador durante os testes realizados. A única dúvida que tivemos foi em relação a um exemplo do descritivo em no mesmo codigo fonte aceitava mais de um bloco `algoritmo ... fimalgoritmo` em sequência, um algoritmo usando um procedimento definido em um algoritmo anterior e depois do `fimalgoritmo` outro procedimento e outro bloco de comandos.

Na nossa implementação, isso foi desconsiderado e na `main()` chamamos `analisar_programa()` uma única vez e logo em seguida exige que o próximo token seja `EOF`, ou seja, o compilador só aceita um único `algoritmo ... fimalgoritmo` por codigo fonte:

```c
// Inicializa o lookahead e inicia o parser
nextToken();
analisar_programa();

// Depois de fimalgoritmo nao deve existir outro token
if (lookahead.type != TOKEN_EOF) {
    erroSintatico(lookahead.line, lookahead);
}
```

Definimos assim na Etapa 1 e então mantivemos na implementação
