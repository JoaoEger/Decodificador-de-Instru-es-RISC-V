#ifndef REPORT_HPP
#define REPORT_HPP

#include "instruction.hpp"
#include <vector>
#include <string>
using namespace std;

string formatarLinhaSaida(const Instruction &inst);
string relatorioCPI(const vector<Instruction> &instrucoes);

#endif