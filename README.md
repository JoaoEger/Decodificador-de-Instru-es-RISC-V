# Decodificador de Instruções RISC-V

Programa em C++ que lê um arquivo de memória de instruções (ROM) em linguagem de máquina RISC-V (32 bits, hexadecimal ou binário), decodifica cada instrução, monta sua representação em assembly e gera um relatório estatístico do programa analisado.

Trabalho da disciplina de Organização de Computadores — UNIVALI (Avaliação Prática M1, Etapa 1 de 2).

Autores: Eduardo Spezia e João Eger.

## Funcionalidades

- Leitura automática de arquivos com instruções em **hexadecimal** ou **binário**, sem precisar informar o formato.
- Ignora linhas em branco, comentários (`#` ou `//`) e o prefixo `0x`.
- Endereço-base configurável (você escolhe de onde a contagem de endereços começa).
- Classificação nos 6 formatos de instrução (`R`, `I`, `S`, `B`, `U`, `J`) e identificação do mnemônico combinando opcode, `funct3` e `funct7`.
- Extração dos campos válidos para cada formato (`rd`, `rs1`, `rs2`, `funct3`, `funct7`, imediato), com reconstrução correta do imediato — inclusive nos formatos `B` e `J`, cujos bits vêm fora de ordem — e extensão de sinal.
- Desmontagem em assembly usando os nomes ABI dos registradores (`zero`, `ra`, `sp`, `s0`, `a0`, ...).
- Cálculo do endereço absoluto de destino em desvios (`beq`, `bne`, ...) e saltos (`jal`).
- Reconhecimento das pseudo-instruções mais comuns (`nop`, `ret`, `mv`, `not`).
- Relatório final com a contagem e o percentual de instruções por formato, e o CPI médio do programa (a partir de uma tabela de CPI por classe de instrução).
- Instruções inválidas são reportadas no `stderr` com o endereço onde ocorrem, sem interromper o processamento do restante do arquivo.

## Estrutura do projeto

```
.
├── headers/            # arquivos .hpp (declarações)
├── src/                # arquivos .cpp (implementação)
│   ├── main.cpp
│   ├── read_file.cpp   # leitura e parsing do arquivo de entrada
│   ├── decode.cpp       # classificação de formato e extração de campos
│   ├── instruction.cpp  # despacho para o extrator de cada formato
│   ├── converter.cpp    # conversões (bits -> número, nomes ABI, assembly)
│   ├── report.cpp       # formatação da saída e relatório estatístico
│   └── cpi_calculator.cpp
├── Makefile
├── machine_code.txt    # exemplo de arquivo de entrada
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

Usando `src/*.cpp` para pegar todos os arquivos de uma vez:

```bash
g++ -std=c++17 -Iheaders -o decodificador src/*.cpp
```

Ou, se preferir listar cada arquivo `.cpp` explicitamente:

```bash
g++ -std=c++17 -Iheaders -o decodificador src/main.cpp src/read_file.cpp src/decode.cpp src/instruction.cpp src/converter.cpp src/report.cpp src/cpi_calculator.cpp
```

## Como executar

O programa espera o caminho do arquivo de instruções como argumento:

```bash
./decodificador caminho/para/arquivo.txt
```

Ao iniciar, ele pede o **endereço-base** (em hexadecimal, sem o prefixo `0x`) de onde as instruções devem ser carregadas:

```
Adicione o endereco inicial das instrucoes (em hexadecimal, apenas os digitos apos 0x Ex: Certo: 80AFD000 | Errado: 0x80AFD000):
```

### Exemplo

Com o arquivo [`machine_code.txt`](machine_code.txt) incluso no repositório:

```bash
./decodificador machine_code.txt
```

Digite `0` (ou qualquer endereço-base desejado) quando solicitado. A saída mostra, para cada instrução, o endereço, a palavra original, o formato, o mnemônico, os campos extraídos e o assembly desmontado, seguida do relatório estatístico:

```
=====================================================================================
Endereco   | Instrucao  | Tipo     | Mnemoni. | Registradores -> Assembly
=====================================================================================
0x00000000 | 0x00500413 | I        | addi     | rd=8 rs1=0 imm=5 opcode=0010011 function3=0  ->  addi s0, zero, 5
0x00000004 | 0x00c58633 | R        | add      | rd=12 rs1=11 rs2=12 opcode=0110011 function3=0 function7=0  ->  add a2, a1, a2
0x00000008 | 0x0064a423 | S        | sw       | rs1=9 rs2=6 imm=8 opcode=0100011 function3=2  ->  sw t1, 8(s1)
...
Total de Instruções: N
CPI Médio: X.XXXXXX
Instruções R-type: N (XX.X%)
...
```

## Formato do arquivo de entrada

- Uma instrução por linha, em hexadecimal (`0x00500413` ou `00500413`) ou binário (`00000000010100000000010000010011`).
- Linhas em branco e comentários (iniciados com `#` ou `//`) são ignorados.
- O formato (hex ou binário) é detectado automaticamente linha a linha — não é preciso misturar apenas um formato por arquivo, embora o recomendado seja manter consistência.

## Testando manualmente

O arquivo [`machine_code.txt`](machine_code.txt) traz um pequeno programa de exemplo (cálculo de Fibonacci) que exercita vários formatos de instrução. Recomenda-se montar também um conjunto de testes próprio, cobrindo pelo menos uma instrução de cada formato (`R`, `I`, `S`, `B`, `U`, `J`) e um desvio com deslocamento negativo, conforme pedido no enunciado do trabalho.
