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

Identificadores, strings e a maior parte dos operadores/delimitadores são armazenados em uma **tabela de símbolos** (`symtab` que é um vetor de strings) e o token guarda apenas o index (`attribute.table_index`), isso usando a estrutura de união de atributos definida na `Token` já definida no descritivo. As palavras reservadas estão em uma lista estática (`palavrasReservadas[]`) e identificadas pela função `ehPalavraReservada()`, identificadores por `findOrAddIdentifier()`. jà os operadores relacionais usam `OpRelAtributo`) em vez da tabela de símbolos, já que são um conjunto pequeno.

Nesse contexto, a função `infoToken()` imprime e grava cada token como `linha# CATEGORIA | lexema`, tanto na tela quanto em `tokens_saida.txt`.

## Analisador Sintático



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
