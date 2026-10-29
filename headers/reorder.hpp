#ifndef REORDER_HPP
#define REORDER_HPP

#include "instruction.hpp"
#include "control.hpp"
#include "correction.hpp"
#include <vector>
#include <string>
using namespace std;

struct Dependencia {
    size_t de;
    size_t para;
    string tipo; // "RAW", "WAR", "WAW" ou "MEM"
};

bool acessaMemoria(const Instruction &inst);
bool leDaMemoria(const Instruction &inst);
bool escreveNaMemoria(const Instruction &inst);

vector<Dependencia> construirGrafo(const vector<Instruction> &instrucoes, const BlocoBasico &bloco);
vector<vector<size_t>> ordensValidas(const vector<size_t> &indices, const vector<Dependencia> &grafo);
vector<LinhaCorrigida> reordenarComNops(const vector<Instruction> &instrucoes);

#endif