# Decodificador de Instruções RISC-V

Programa em C++ que lê um arquivo de memória de instruções (ROM) em linguagem de máquina RISC-V (32 bits, hexadecimal ou binário), decodifica cada instrução, detecta os conflitos de dados e de controle de um pipeline de 5 estágios sem proteção e gera versões corrigidas do programa: uma só com `nop`s e outra com reordenação de instruções.

Trabalho da disciplina de Arquitetura e Organização de Computadores — UNIVALI:

- **Prática M1:** decodificação das instruções e relatório estatístico.
- **Prática M2:** conflitos de dados e de controle no pipeline (requisitos R1 a R8).

Autores: Eduardo Spezia e João Eger.

## Funcionalidades

### Prática M1 — decodificação

- Leitura automática de arquivos com instruções em **hexadecimal** ou **binário**, sem precisar informar o formato.
- Ignora linhas em branco, comentários (`#` ou `//`) e o prefixo `0x`.
- Endereço-base configurável (você escolhe de onde a contagem de endereços começa).
- Classificação nos 6 formatos de instrução (`R`, `I`, `S`, `B`, `U`, `J`) e identificação do mnemônico combinando opcode, `funct3` e `funct7`.
- Extração dos campos válidos para cada formato (`rd`, `rs1`, `rs2`, `funct3`, `funct7`, imediato), com reconstrução correta do imediato — inclusive nos formatos `B` e `J`, cujos bits vêm fora de ordem — e extensão de sinal.
- Desmontagem em assembly usando os nomes ABI dos registradores (`zero`, `ra`, `sp`, `s0`, `a0`, ...).
- Cálculo do endereço absoluto de destino em desvios (`beq`, `bne`, ...) e saltos (`jal`).
- Reconhecimento das pseudo-instruções mais comuns (`nop`, `ret`, `mv`, `not`).
- Relatório com a contagem e o percentual de instruções por formato, e o CPI médio do programa.
- Instruções inválidas são reportadas no `stderr` com o endereço onde ocorrem, sem interromper o processamento.

### Prática M2 — conflitos no pipeline

Premissas (as mesmas do enunciado): pipeline IF/ID/EX/MEM/WB sem forwarding, sem detecção de conflitos e sem predição; banco de registradores escrito na 1ª metade do ciclo e lido na 2ª (conflito quando `d = 1` ou `d = 2`, com `3 − d` nops); desvios resolvidos no ID, com a instrução seguinte (slot de atraso) sempre executada; `x0` nunca gera conflito.

| Requisito | O que o programa faz | Onde |
|---|---|---|
| **R1** Conflitos de dados | Lista cada par produtor/consumidor com `d ≤ 2`: endereços, registrador, distância e nops necessários. Considera só leitura após escrita (RAW), só os campos que cada formato realmente usa e sempre o produtor mais recente. | `hazards.cpp` |
| **R2** Conflitos de controle | Lista cada desvio (`B`, `jal`, `jalr`) e a instrução que ocupa o seu slot, monta os blocos básicos (todo alvo inicia um bloco, todo desvio encerra um) e detecta os conflitos de dados que atravessam o desvio, nos dois caminhos. | `control.cpp` |
| **R3** Nops de dados | Monta o programa novo instrução por instrução e coloca antes de cada uma o mínimo de nops que ela precisa, medindo as distâncias já com os nops inseridos. | `correction.cpp` |
| **R4** Nops de controle | Coloca um nop no slot de atraso de cada desvio. | `correction.cpp` |
| **R5** Reordenação | Dentro de cada bloco básico, escolhe a ordem com menos nops respeitando as dependências RAW, WAR, WAW e de memória, e preenche o slot com uma instrução útil de antes do desvio quando ela é segura nos dois caminhos. Depois reverifica a entrada dos caminhos. | `reorder.cpp` |
| **R6** Recálculo dos desvios | Recalcula o deslocamento de cada branch e `jal` a partir das posições novas e recodifica a instrução em linguagem de máquina. | `relocation.cpp`, `encoder.cpp` |
| **R7** Arquivos de saída | Grava os dois programas corrigidos no mesmo formato da entrada, mais uma listagem legível de cada um. | `output.cpp` |
| **R8** Relatório de custo | Mostra, para cada técnica, a contagem original, a final, o acréscimo e o percentual, e compara as duas. | `output.cpp` |

Alguns detalhes tratados:

- O `ecall` lê `a7` e `a0` implicitamente, então ele é tratado como consumidor desses registradores e nenhuma instrução troca de lado com ele na reordenação.
- O slot de uma chamada de função (`jal`/`jalr` que escreve `rd`) não recebe instrução útil: o `ret` volta para o endereço do slot, e a instrução executaria duas vezes.
- Limitações: o alvo de `jalr` depende de registrador e não pode ser recalculado; pares `auipc` + `addi`/`lw` também não são ajustados. O programa mostra um aviso nesses casos. Blocos com mais de 8 instruções não são reordenados internamente (apenas o slot é preenchido), para a busca não ficar lenta.

## Estrutura do projeto

```
.
├── headers/               # arquivos .hpp (declarações)
├── src/                   # arquivos .cpp (implementação)
│   ├── main.cpp
│   ├── read_file.cpp      # leitura e parsing do arquivo de entrada
│   ├── decode.cpp         # classificação de formato e extração de campos
│   ├── instruction.cpp    # despacho para o extrator de cada formato
│   ├── converter.cpp      # conversões (bits -> número, nomes ABI, assembly)
│   ├── report.cpp         # formatação da saída e relatório estatístico (M1)
│   ├── cpi_calculator.cpp
│   ├── hazards.cpp        # R1: conflitos de dados
│   ├── control.cpp        # R2: desvios, slots, blocos básicos e fronteiras
│   ├── correction.cpp     # R3 + R4: correção somente com nops
│   ├── reorder.cpp        # R5: reordenação dentro dos blocos básicos
│   ├── relocation.cpp     # R6: novos endereços e deslocamentos
│   ├── encoder.cpp        # R6: recodifica o imediato de branches e jal
│   └── output.cpp         # R7 + R8: arquivos de saída e relatório de custo
├── testes/                # casos de teste próprios da M2
├── Makefile
├── fibonacci_function.txt # exemplo: sequência de Fibonacci (hex)
├── sum_function.txt       # exemplo: somas acumuladas com laço (hex)
├── leaf_function.txt      # exemplo: função folha f = (g + h) - (i + j) (binário)
└── README.md
```

## Como compilar

### Opção 1 — usando o `make` (recomendado)

Na raiz do projeto:

```bash
make
```

Isso gera o executável `decodificador`. Para limpar os arquivos objeto e o executável:

```bash
make clean
```

### Opção 2 — compilando manualmente

```bash
g++ -std=c++17 -Iheaders -o decodificador src/*.cpp
```

## Como executar

O programa espera o caminho do arquivo de instruções como argumento:

```bash
./decodificador caminho/para/arquivo.txt
```

Ao iniciar, ele pede o **endereço-base** (em hexadecimal, sem o prefixo `0x`). Para programas exportados do RARS, use `00400000`:

```
Adicione o endereco inicial das instrucoes (em hexadecimal, apenas os digitos apos 0x Ex: Certo: 80AFD000 | Errado: 0x80AFD000):
```

### O que aparece no console

1. A tabela de decodificação e o relatório estatístico (M1).
2. `CONFLITOS DE DADOS` (R1).
3. `CONFLITOS DE CONTROLE`, `BLOCOS BASICOS` e `CONFLITOS NA FRONTEIRA DO DESVIO` (R2).
4. `PROGRAMA CORRIGIDO (SOMENTE NOPS)` e `PROGRAMA CORRIGIDO (NOPS + REORDENACAO)`, cada um com a conferência de que não sobrou conflito e de que os desvios foram recalculados.
5. `ARQUIVOS GERADOS` (R7).
6. `CUSTO` (R8).

Exemplo do conflito e do custo para o `sum_function.txt`:

```
CONFLITOS DE DADOS
  0x00400010  add t2, t2, t0            <- 0x0040000c  addi t2, zero, 0          [t2, d=1, 2 nops]
  0x00400018  add t4, s0, t3            <- 0x00400014  slli t3, t0, 2            [t3, d=1, 2 nops]
  ...

CONFLITOS DE CONTROLE
  0x00400024  blt t0, t1, 0x00400010    slot ocupado por: 0x00400028  lw a0, 36(s0)
  ...

CUSTO
original .............. 17 instrucoes
somente nops .......... 32 instrucoes (+15, +88,2%)
nops + reordenacao .... 26 instrucoes (+9, +52,9%)
comparacao ............ a reordenacao usa 6 instrucoes a menos (18,8% menor que somente nops)
```

### Arquivos gerados

Os arquivos são criados na mesma pasta do arquivo de entrada. Para `sum_function.txt`:

| Arquivo | Conteúdo |
|---|---|
| `sum_function_nops.txt` | Arquivo 1: programa corrigido só com nops (R3 + R4) |
| `sum_function_nops_listagem.txt` | Listagem: endereço, código, assembly e o que foi inserido |
| `sum_function_reordenado.txt` | Arquivo 2: programa corrigido com reordenação + nops (R5) |
| `sum_function_reordenado_listagem.txt` | Listagem: endereço, código, assembly e o que foi inserido ou movido |

Os arquivos de programa ficam no mesmo formato da entrada (hexadecimal ou binário) e podem ser lidos de volta pelo próprio decodificador:

```bash
./decodificador sum_function_nops.txt
```

Na releitura, nenhuma instrução deve aparecer como inválida e `CONFLITOS DE DADOS` deve mostrar `nenhum`.

## Formato do arquivo de entrada

- Uma instrução por linha, em hexadecimal (`0x00500413` ou `00500413`) ou binário (`00000000010100000000010000010011`).
- Linhas em branco e comentários (iniciados com `#` ou `//`) são ignorados.
- O formato (hex ou binário) é detectado automaticamente linha a linha.

## Testes

### Exemplos da disciplina

| Arquivo | Programa | Formato | Original | Somente nops | Nops + reordenação |
|---|---|---|---|---|---|
| [`fibonacci_function.txt`](fibonacci_function.txt) | Sequência de Fibonacci | hex | 41 | 79 | 71 |
| [`sum_function.txt`](sum_function.txt) | Somas acumuladas com laço | hex | 17 | 32 | 26 |
| [`leaf_function.txt`](leaf_function.txt) | Função folha `f = (g + h) - (i + j)` | binário | 19 | 28 | 28 |

### Casos de teste próprios (`testes/`)

Cada arquivo começa com comentários explicando o caso e o assembly de cada instrução.

| Arquivo | Caso | Original | Somente nops | Nops + reordenação |
|---|---|---|---|---|
| [`cadeia_dependencias.txt`](testes/cadeia_dependencias.txt) | Cadeia de dependências sem nenhuma instrução independente | 4 | 10 | 10 |
| [`laco_desvio_tras.txt`](testes/laco_desvio_tras.txt) | Laço com desvio condicional para trás | 6 | 9 | 9 |
| [`desvio_frente.txt`](testes/desvio_frente.txt) | Desvio para a frente e `jal` cujo alvo lê `ra` | 8 | 15 | 15 |
| [`reordenacao_zera_nops.txt`](testes/reordenacao_zera_nops.txt) | Reordenação elimina todos os nops | 8 | 11 | 8 |
| [`chamada_funcao.txt`](testes/chamada_funcao.txt) | Chamada de função: o slot do `jal ra` fica com nop | 8 | 13 | 13 |
| [`ecall_ordem.txt`](testes/ecall_ordem.txt) | O `ecall` não pode passar para antes de quem prepara `a0` | 5 | 11 | 10 |

```bash
./decodificador testes/reordenacao_zera_nops.txt
```
