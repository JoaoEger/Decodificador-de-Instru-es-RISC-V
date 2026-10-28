#ifndef CORRECTION_HPP
#define CORRECTION_HPP

#include "instruction.hpp"
#include <vector>
#include <string>
using namespace std;

// Uma linha do programa corrigido. Lembra de onde veio pra listagem
// marcar o que foi inserido e pro recálculo dos desvios (R6)
struct LinhaCorrigida {
    Instruction inst;
    int indiceOriginal;   // posição no programa original, -1 se for nop inserido
    string marcacao;      // vazio pra instrução original
};

vector<LinhaCorrigida> corrigirComNops(const vector<Instruction> &instrucoes);
void imprimirProgramaCorrigido(const vector<LinhaCorrigida> &programa, unsigned int enderecoInicial);

#endif
