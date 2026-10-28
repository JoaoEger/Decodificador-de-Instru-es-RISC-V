#include "correction.hpp"
#include "hazards.hpp"
#include "control.hpp"
#include "converter.hpp"

// nop = addi zero, zero, 0 = 0x00000013
static LinhaCorrigida criarNop(string motivo) {
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
static int nopsNecessarios(const vector<LinhaCorrigida> &saida, const Instruction &inst) {
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
// depois de cada desvio pra ocupar o slot de atraso (R4)
vector<LinhaCorrigida> corrigirComNops(const vector<Instruction> &instrucoes) {
    vector<Desvio> desvios = detectarDesvios(instrucoes);
    vector<LinhaCorrigida> saida;

    for (size_t i = 0; i < instrucoes.size(); i++) {
        const Instruction &inst = instrucoes[i];
        int nops = nopsNecessarios(saida, inst);

        // Caminho tomado: com nop no slot fica "desvio, nop, alvo", então o
        // desvio está a d=2 do alvo. Só o jal escreve registrador (ra), e se o
        // alvo ler esse registrador precisa de mais 1 nop
        for (const Desvio &d : desvios) {
            if (d.indiceAlvo != (int)i) continue;
            int escrito = registradorEscrito(instrucoes[d.indice]);
            for (int reg : registradoresLidos(inst)) {
                if (reg == escrito) nops = max(nops, 1);
            }
        }

        for (int k = 0; k < nops; k++) saida.push_back(criarNop("conflito de dados"));

        LinhaCorrigida linha;
        linha.inst = inst;
        linha.indiceOriginal = (int)i;
        linha.marcacao = "";
        saida.push_back(linha);

        if (ehDesvio(inst)) saida.push_back(criarNop("slot de atraso"));
    }
    return saida;
}

// O endereço novo é só a posição no programa novo. Os desvios ainda
// mostram o alvo antigo: o recálculo dos deslocamentos é o R6
void imprimirProgramaCorrigido(const vector<LinhaCorrigida> &programa, unsigned int enderecoInicial) {
    for (size_t k = 0; k < programa.size(); k++) {
        const LinhaCorrigida &linha = programa[k];
        string assembly = linha.indiceOriginal == -1 ? "nop" : paraAssembly(linha.inst);
        cout << "  " << toHex(enderecoInicial + 4 * k, 8) << "  " << binaryToHex(linha.inst.full_instruction)
             << "  " << left << setw(26) << assembly << linha.marcacao << "\n";
    }
}
