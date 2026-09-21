#ifndef CPICALCULATOR_HPP
#define CPICALCULATOR_HPP

#include <iostream>
#include <vector>
#include <map>
#include "instruction.hpp"
using namespace std;

static const map<string, double> tabelaCPI = {
    {"R", 1.0},
    {"I", 1.2},
    {"S", 2.0},
    {"B", 2.5},
    {"U", 1.0},
    {"J", 3.1}
};

double calcularCPIMedio(const vector<Instruction> &instrucoes);

#endif