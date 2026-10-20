#ifndef HAZARDS_HPP
#define HAZARDS_HPP

#include "instruction.hpp"
#include <vector>
using namespace std;

// Um par produtor/consumidor que viola a distância mínima do pipeline.
// Guarda índices (e não ponteiros) no vetor de instruções, pra continuar
// válido mesmo se o vetor for copiado ou realocado nas próximas etapas
struct Hazard {
    size_t consumidor;
    size_t produtor;
    int registrador;
    int distancia;   // d
    int nops;        // 3 - d
};

int registradorEscrito(const Instruction &inst);
vector<int> registradoresLidos(const Instruction &inst);

vector<Hazard> detectarHazards(const vector<Instruction> &instrucoes);
void imprimirHazards(const vector<Instruction> &instrucoes, const vector<Hazard> &hazards);

#endif
