#include "cpi_calculator.hpp"

// Média ponderada pela ocorrência de cada formato: somar o CPI de cada
// instrução individualmente e dividir pelo total é matematicamente igual a
// (qtd_R * CPI_R + qtd_I * CPI_I + ...) / total, só que sem precisar contar antes
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