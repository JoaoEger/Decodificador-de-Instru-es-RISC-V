#include "cpi_calculator.hpp"

double calcularCPIMedio(const vector<Instruction> &instrucoes){
    if (instrucoes.empty()) return 0.0;

    double somaPonderada = 0.0;
    int consideradas = 0;

    for (const Instruction &inst : instrucoes) {
        map<string, double>::const_iterator i = tabelaCPI.find(inst.type);
        if (i == tabelaCPI.end()) {
            continue;
        }
        somaPonderada += i->second;
        consideradas++;
    }

    if (consideradas == 0) return 0.0;
    return somaPonderada / consideradas;
}