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

Na Fase 2, os programas compatíveis também geram `mepa_cod.txt`. Para compilar o interpretador e executar esse arquivo no CodeSpace(Linux):

```bash
gcc -Wall -Wno-unused-result -g -Og mepa.c -o mepa
./mepa -r -i mepa_cod.txt
```

Também incluímos um `Makefile` para compilar e executar as baterias de testes:

```bash
make help
make build
make mepa
make erros
make clean
```
RECOMENDAMOS USAR O MAKEFILE PELO CODESPACE, POIS NÃO TEM DEPENDÊNCIAS EXTRAS PRA FUNCIONAR.

## Analisador Léxico

O analisador lex foi implementado pensando no `obterToken()`, que percorre o código fonte carregado pela `main()` em um buffer em memória e vai passando pelos caracteres por ponteiro (`bufPos`), usando as funções auxiliares de `charAtual`, `proximoChar`, `avancarChar`.

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

Na Fase 2 também foi incluída `analisar_expr_e()`, para dar prioridade ao operador `E` em relação ao `OU`, como consta na gramática adequada.

Procedimentos e funções são reconhecidos por `analisar_subrotinas()`. Suas declarações são processadas respectivamente por `analisar_declaracao_procedimento()` e `analisar_declaracao_funcao()`, enquanto `analisar_lista_parametros()` trata os parâmetros e seus tipos.

Entre os **erros sintáticos** identificados estão:

- **Estrutura incorreta do programa:** ausência ou troca de palavras obrigatórias, como `algoritmo`, `inicio` e `fimalgoritmo`.
- **Declarações incorretas:** falta de `:` entre o identificador e o tipo, ou delimitadores ausentes em parâmetros e vetores.
- **Expressões e comandos incompletos:** falta de operandos, parênteses ou colchetes de fechamento, além de comandos que não seguem a estrutura prevista na gramática.
- **Blocos não finalizados:** ausência dos fechamentos `fimse`, `fimpara`, `fimenquanto`, `fimprocedimento` e `fimfuncao`.
- **Procedimentos sem parâmetros:** conforme o Anexo I, devem ser declarados e chamados sem parênteses. Os testes `erro_sint1.txt` e `erro_sint2.txt` verificam, respectivamente, a declaração `procedimento rotina()` e a chamada `rotina()`.
- **Conteúdo após o fim do programa:** a `main()` espera `EOF` logo depois de `fimalgoritmo`, pois adotamos um único bloco `algoritmo ... fimalgoritmo` por arquivo.

Por fim, ao encontrar uma construção que não corresponde à gramática esperada, `erroSintatico()` imprime uma mensagem com a linha e o token incorreto e encerra o processo. Erros de tipo, identificadores não declarados e uso incorreto de funções ou procedimentos são tratados separadamente por `erroSemantico()`.

## Analisador Semântico

Para a análise semântica, nós mantivemos a estrutura `TNo` passada no descritivo da professora, em uma minitabela de símbolos implementada como lista encadeada. Ela guarda o identificador, a ordem de declaração como endereço e o tipo da variável. Como também precisamos de informações sobre vetores, uso das variáveis e parâmetros, criamos a estrutura auxiliar `InfoVar`, sem alterar os campos de `TNo`. Para funções e procedimentos, utilizamos ainda uma tabela auxiliar de rotinas (`InfoRotina`).

As principais verificações foram divididas em:

- **Declaração de variáveis:** `no_declarado()` e `busca_tabela_simbolos()` verificam se o identificador foi declarado, enquanto `busca_info()` recupera suas informações.
- **Tipos e expressões:** as funções `analisar_*` devolvem os tipos encontrados para verificar atribuições, operações, condições e retornos.
- **Identificadores e rotinas:** verificamos nomes repetidos, palavras reservadas, quantidade e tipos dos argumentos e o uso correto de funções e procedimentos.
- **Uso das variáveis:** `confere_variaveis_nao_usadas()` identifica, ao final do programa, variáveis que receberam valor, mas nunca foram utilizadas.
- **Retorno de funções:** conferimos a compatibilidade do tipo retornado e, ao finalizar a declaração, verificamos se existe pelo menos um `retorne`. Essa verificação não analisa todos os caminhos possíveis de execução.

A análise semântica acontece junto das funções do analisador sintático. O fluxo de um erro semântico parte dessas verificações: quando alguma regra não é atendida, a função responsável chama `erroSemantico()`, que informa a linha, o motivo e, quando necessário, o identificador. Se houver erro, a compilação é encerrada e não passa para a geração de código.

## Gerador de Código Intermediário

Para essa implementação, nós consideramos a rotulação junto das soluções de cada comando. A função `proximo_rotulo()` gera rótulos consecutivos, que são guardados antes de analisar blocos internos para não misturar os destinos de desvios em comandos aninhados. Assim, condicionais e repetições utilizam instruções como `DSVF` e `DSVS`, com os pontos de destino marcados por `L1: NADA`, `L2: NADA` e assim por diante.

O gerador de código intermediário MEPA foi integrado às mesmas funções `analisar_*` e ao fluxo de `consome()` do analisador sintático. Conforme os comandos e expressões são reconhecidos e verificados semanticamente, usamos as funções `emite()`, `emite_num()`, `emite_nome()` e `emite_rot()` para guardar as instruções em um buffer. Por exemplo, uma atribuição gera as operações da expressão e depois `ARMZ` para armazenar o resultado. Também utilizamos as operações de pilha para expressões, os desvios para `se`, `enquanto` e `para` e as instruções de rotinas para funções e procedimentos simples.

No final, `grava_mepa()` escreve o arquivo `mepa_cod.txt`, com `INPP`, reserva de memória por `AMEM`, instruções do programa, `DMEM` e `PARA`. Como o enunciado pede variáveis referenciadas pelos nomes, adicionamos linhas `; VAR nome deslocamento [nivel]` no começo do TXT. Dessa forma, o compilador pode gerar `CRVL a` e `ARMZ b`, que o MEPA adaptado consegue reconhecer. O deslocamento utilizado na execução é separado do endereço da `TNo`, que, conforme o descritivo, guarda a ordem de declaração das variáveis. Para vetores e operações como `MOD`, também usamos instruções existentes e posições temporárias, sem criar novas instruções da máquina.

Durante as aulas e considerando também o MEPA passado pela professora, observamos que os códigos executados pela máquina são mais simples. Ou seja, não é porque um programa passou pelos analisadores léxico, sintático e semântico que necessariamente poderá ser executado pelo MEPA. Respeitamos essa limitação: operações com reais, caracteres e divisão real (`/`) podem ser semanticamente válidas, mas não geram `mepa_cod.txt` executável. O interpretador trabalha principalmente com valores inteiros e lógicos; a divisão inteira (`\`) utiliza `DIVI`. Textos literais isolados em `escreva`/`escreval` podem aparecer como comentários no TXT, mas não são impressos pela máquina.

## MEPA (Máquina de Execução)

Para executar o código intermediário, utilizamos como base o **InterprMEPA-2.0** do Bruno Muller Junior, disponibilizado pela professora durante o desenvolvimento do projeto. O pacote original (`InterprMEPA-2.0.tar.bz2`) possui diferentes arquivos para a máquina, a interface e o leitor das instruções; a leitura original utiliza `leMepa.l` e `leMepa.y`, com Flex e Bison. Porém, utilizar diretamente esse MEPA poluiria o repositorio e poderia até mesmo confundir a gente na hora da implementação do código no padrão MEPA com as restrições que a gente viu em sala. Como solução reunimos a implementação em um único `mepa.c` e adaptamos a parte de leitura, mantendo as **34 instruções MEPA** do projeto base.

As principais adaptações e seus motivos foram:

- **Leitura em um único arquivo C:** substituímos o leitor feito com Flex/Bison por funções em C, como `leComandosMepa()`, para compilar o interpretador diretamente com `gcc` e facilitar a execução.
- **Variáveis referenciadas por nome:** a MEPA original utiliza operandos numéricos (nível e deslocamento), mas o descritivo da Fase 2 pede que as variáveis apareçam pelos nomes no código intermediário. Por isso, o `compilador.c` grava linhas no formato `; VAR nome deslocamento [nivel]` e continua emitindo instruções como `CRVL a` e `ARMZ a`.
- **Uso de `; VAR`:** escolhemos o prefixo `;` porque ele não é utilizado como operador na nossa GLC do MiniVisualg. No arquivo MEPA, ele funciona como marcador de comentário, e o leitor adaptado reconhece especificamente `; VAR` como uma declaração auxiliar. **Esse formato não é uma instrução padrão da MEPA**, mas foi a solução que encontramos para atender à exigência dos nomes sem modificar as instruções da máquina.
- **Conversão para endereços:** `leComandosMepa()` percorre o TXT em duas passadas. Na primeira, `registraVariavel()` coleta as declarações `; VAR`; na segunda, o leitor resolve cada nome para seu nível e deslocamento e carrega as instruções. Assim, a máquina continua executando os operandos numéricos internamente.
- **Rotinas e memória:** mantivemos instruções como `AMEM`, `DMEM`, `ENPR`, `CHPR` e `RTPR` para as rotinas simples, com variáveis globais no nível 0 e parâmetros/variáveis locais no quadro de ativação da rotina. Rótulos e desvios continuam utilizando as instruções da própria MEPA.

Por exemplo, o começo do arquivo gerado pode conter:

```text
; VAR a 0
INPP
AMEM 1
DSVS L1
L1: NADA
CRCT 6
ARMZ a
CRVL a
IMPR
DMEM 1
PARA
```

A linha `; VAR a 0` informa ao leitor que `a` corresponde ao deslocamento 0 do nível global. Ela **não é executada** como instrução; serve somente para que `CRVL a` e `ARMZ a` sejam convertidos para os endereços utilizados pela máquina. O salto `DSVS L1`, embora dispensável nesse exemplo sem rotinas, faz parte do padrão do gerador para separar as rotinas do bloco principal.

**Limitações consideradas:** mantivemos o foco nas operações básicas sobre inteiros e lógicos, condições, repetições, vetores inteiros e rotinas simples. O projeto passado no Moodle não oferece execução de números reais e strings como valores da máquina. Por isso, programas semanticamente válidos que dependem desses recursos, a solução que a gente propôs simplesmente não gera codigo para o MEPA `mepa_cod.txt`. Em `escreva` e `escreval`, a saída numérica usa `IMPR`, sem diferenciar a quebra de linha; textos literais isolados são registrados como comentários, não como saída executada. A implementação também limita o `passo` do comando `para` a uma constante inteira.

O código original utilizado como referência está em [InterprMEPA-2.0](https://www.inf.ufpr.br/bmuller/assets/ci1211/Recursos/InterprMEPA-2.0.tar.bz2), identificado no cabeçalho do `mepa.c` como projeto de Bruno Muller Junior.

## Relatório de Design e Implementação

O repositório está organizado em:

- `compilador.c`: todo o código do compilador (léxico + sintático + semântico + gerador de código intermediário + main).
- `mepa.c`: interpretador MEPA adaptado para executar o arquivo intermediário gerado.
- `Makefile`: compila (`make build`), mostra os comandos (`make help`), executa os testes MEPA (`make mepa`) e os testes de erros (`make erros`) e remove os arquivos gerados (`make clean`).
- `etapa1_gramatica.txt`: Etapa 1 para servir de base na implementação (define as expressões regulares e a gramática adequada da linguagem MiniVisualg).
- `Testes/TestesFase1/`: casos de teste da Fase 1 (`teste1.txt` a `teste16.txt`), mantidos para execução individual.
- `Testes/anexo1/`: 21 testes (`teste1.txt` a `teste21.txt`) baseados nos exemplos do Anexo I; alguns validam apenas os analisadores e outros também permitem gerar MEPA.
- `Testes/testesCadaUm/erros/`: dois testes léxicos, dois sintáticos, dois semânticos e dois programas válidos que não geram MEPA (`sem_mepa1.txt` e `sem_mepa2.txt`).
- `Testes/testesCadaUm/corretos/`: 18 testes que passam pelos analisadores, geram código intermediário e podem ser executados no MEPA.
- `tokens_saida.txt`: arquivo gerado com os tokens reconhecidos pelo compilador.
- `mepa_cod.txt`: arquivo gerado com as instruções MEPA quando o programa utiliza recursos suportados.

    Em suma , a solução foi pensada em cima da linha de compilação do descritivo, com estrutura: `obterToken()` / `infoToken()` / `nextToken()`, com os analisadores lexico e sintático dentro do arquivo `compilador.c` e comunicando por chamadas de função e variável global `lookahead`. Na Fase 2, mantivemos essa estrutura e integramos a análise semântica e a emissão do código intermediário às mesmas funções do analisador sintatico.

Outra solução possível, que chegamos a analisar, era separando o analisador léxico e o analisador sintático em dois códigos (por exemplo, `lexico.c` com seu `lexico.h` e `sintatico.c` com seu `sintatico.h`), definindo um `.h` compartilhado apenas com os protótipos de `obterToken()` e `infoToken()` e os tipos `Token` e `TokenNome`.

## Relatório de Uso de IA

Na Fase 1, usamos o Claude Sonnet 5 em Baixo com pensamento estendido e o ChatGPT. E na Fase 2 o Claude SOnnet 5.5 e o ChatGPT. Na segunda fase o uso de IA foi fundamental nas adaptações e correções pontuais. Abaixo detalhamos os usos:
- Para ajudar com a arquitetura inicial do projeto e para inicio da Fase 2
- Para gerar os casos de teste (`Testes/`) a partir da linguagem exemplificadas no Anexo I e outras mais para os casos de execução MEPA
- Pedimos uma base em um único arquivo `mepa.c`, com TODOs, a partir do pacote `InterprMEPA-2.0` disponibilizado pela professora no Moodle da disciplina e de URLs com implementações de MEPA.
- E também nos auxiliou para encontrar erros e ajudar com a implementação de partes isoladas.


## Relatório de Bugs e Erros

Nos testes realizados, não encontramos outros erros nos casos cobertos, além das limitações descritas abaixo. Uma dúvida que tivemos na fase 1foi em relação a um exemplo do descritivo em que o mesmo código fonte apresentava mais de um bloco `algoritmo ... fimalgoritmo` em sequência: um algoritmo utilizava um procedimento definido anteriormente e, depois de `fimalgoritmo`, apareciam outro procedimento e outro bloco de comandos.

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

Além disso, encontramos algumas limitações importantes como:

- **Tipos não suportados pelo MEPA:** operações que dependem de números reais ou caracteres podem passar pelas análises, mas não geram código executável pelo interpretador adaptado. O mesmo vale para `/`, que representa divisão real no MiniVisualg; a máquina possui `DIVI` para divisão inteira, utilizada com `\`. Literais de texto isolados em `escreva`/`escreval` são tratados como comentários, sem impressão do texto.
- **Saída de texto:** o compilador reconhece `escreva` e `escreval`, mas ambos utilizam `IMPR` no MEPA, sem diferenciar a quebra de linha.
- **Comando `para`:** optamos por um passo inteiro constante, incluindo valores negativos, conforme os exemplos estudados em aula.

Essas limitações foram consideradas durante a Fase 2 para manter a implementação compatível com o MEPA disponibilizado e com os exemplos simples trabalhados na disciplina e aconselhados pela professora.