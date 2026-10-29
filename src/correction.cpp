#include "correction.hpp"
#include "hazards.hpp"
#include "control.hpp"
#include "converter.hpp"
#include "realocation.hpp"

// nop = addi zero, zero, 0 = 0x00000013
LinhaCorrigida criarNop(string motivo) {
    string binario = "00000000000000000000000000010011";
    LinhaCorrigida nop;
    nop.inst = decode_instruction(binario, 0);
    nop.indiceOriginal = -1;
    nop.marcacao = "<- nop inserido: " + motivo;
    return nop;
}

// Olha as duas últimas instruções já colocadas no programa novo (nops
// inclusos, eles não escrevem nada) e vê quantos nops a instrução precisa.
// Usa o maior valor entre os registradores lidos, não a soma: os mesmos
// nops afastam a instrução de todos os produtores ao mesmo tempo
int nopsNecessarios(const vector<LinhaCorrigida> &saida, const Instruction &inst) {
    int nops = 0;
    for (int reg : registradoresLidos(inst)) {
        for (int d = 1; d <= 2 && d <= (int)saida.size(); d++) {
            if (registradorEscrito(saida[saida.size() - d].inst) == reg) {
                nops = max(nops, 3 - d);
                break;
            }
        }
    }
    return nops;
}

// R3 + R4 numa passada só: monta o programa novo instrução por instrução,
// colocando antes de cada uma os nops que ela precisa (R3) e um nop
// depois de cada desvio pra ocupar o slot de atraso (R4). No fim, o R6
// (relocation.cpp) acerta os nops do caminho tomado e os deslocamentos
vector<LinhaCorrigida> corrigirComNops(const vector<Instruction> &instrucoes) {
    vector<LinhaCorrigida> saida;

    for (size_t i = 0; i < instrucoes.size(); i++) {
        const Instruction &inst = instrucoes[i];
        int nops = nopsNecessarios(saida, inst);

        for (int k = 0; k < nops; k++) saida.push_back(criarNop("conflito de dados"));

        LinhaCorrigida linha;
        linha.inst = inst;
        linha.indiceOriginal = (int)i;
        linha.marcacao = "";
        saida.push_back(linha);

        if (ehDesvio(inst)) saida.push_back(criarNop("slot de atraso"));
    }

    // Nops extras do caminho tomado (jal -> alvo) e recálculo dos desvios (R6)
    finalizarPrograma(saida, instrucoes);
    return saida;
}

// O endereço novo é a posição no programa novo. Os desvios já saem com o
// deslocamento recalculado (R6), então o alvo mostrado é o novo
void imprimirProgramaCorrigido(const vector<LinhaCorrigida> &programa, unsigned int enderecoInicial) {
    for (size_t k = 0; k < programa.size(); k++) {
        const LinhaCorrigida &linha = programa[k];
        string assembly = linha.indiceOriginal == -1 ? "nop" : paraAssembly(linha.inst);
        cout << "  " << toHex(enderecoInicial + 4 * k, 8) << "  " << binaryToHex(linha.inst.full_instruction)
             << "  " << left << setw(26) << assembly << linha.marcacao << "\n";
    }
}