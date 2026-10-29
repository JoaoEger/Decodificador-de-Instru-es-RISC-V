#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <string>
using namespace std;

// Codificador (inverso do decode.cpp) para as instruções de desvio, usado
// pelo R6 pra reescrever o imediato depois que os endereços mudam.

// O deslocamento cabe no campo? B: 13 bits com sinal (±4 KiB), J: 21 bits
// com sinal (±1 MiB). O bit 0 não é guardado, então o valor precisa ser par.
bool cabeNoDeslocamento(const string &tipo, long deslocamento);

// Recebe a palavra de 32 bits em string binária (a mesma de full_instruction)
// e devolve outra palavra, igual à original exceto pelo imediato, que passa
// a ser "deslocamento". "tipo" é "B" (branch) ou "J" (jal).
string codificarDeslocamento(const string &bits, const string &tipo, long deslocamento);

#endif