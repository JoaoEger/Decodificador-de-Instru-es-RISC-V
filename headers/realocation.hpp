#ifndef REALOCATION_HPP
#define REALOCATION_HPP

#include "instruction.hpp"
#include "correction.hpp"
#include <vector>
using namespace std;

void finalizarPrograma(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes);
void completarCaminhosTomados(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes);
void relocarDesvios(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes);
void avisosDeRelocacao(const vector<Instruction> &instrucoes);

int verificarRelocacao(const vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes, int &conferidos);

#endif