#ifndef CONTROL_HPP
#define CONTROL_HPP

#include "instruction.hpp"
#include "hazards.hpp"
#include <vector>
using namespace std;

// Uma instrução que muda o fluxo (branch, jal ou jalr)
struct Desvio {
    size_t indice;      // posição do desvio no vetor de instruções
    bool condicional;   // true pra branch (tipo B), false pra jal/jalr
    int indiceAlvo;     // posição do alvo no vetor, ou -1 se não der pra saber
};

// Trecho [inicio, fim] de índices que sempre executa inteiro
struct BlocoBasico {
    size_t inicio;
    size_t fim;
};

// Conflito de dados que só aparece ao entrar num dos caminhos do desvio
struct ConflitoFronteira {
    size_t desvio;          // índice do desvio que gerou o caminho
    bool caminhoTomado;     // true = caminho do alvo, false = caminho que segue a memória
    Hazard hazard;
};

bool ehDesvio(const Instruction &inst);
vector<Desvio> detectarDesvios(const vector<Instruction> &instrucoes);
vector<BlocoBasico> calcularBlocos(const vector<Instruction> &instrucoes, const vector<Desvio> &desvios);
vector<ConflitoFronteira> conflitosNaFronteira(const vector<Instruction> &instrucoes, const Desvio &desvio);

void imprimirConflitosControle(const vector<Instruction> &instrucoes, const vector<Desvio> &desvios,
                               const vector<BlocoBasico> &blocos, const vector<ConflitoFronteira> &fronteira);

#endif
