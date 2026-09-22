#include <iostream>
#include <vector>
#include "instruction.hpp"
#include "read_file.hpp"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Execute o programa passando como parametro o caminho do arquivo em que esta as instrucoes!\n";
        return 1;
    }

    string caminhoInstrucoes = argv[1];
    vector<LinhaLida> linhas = lerArquivoInstrucoes(caminhoInstrucoes);
    
    vector<Instruction> instrucoes;
    instrucoes.reserve(linhas.size());
    for (LinhaLida &linha : linhas) {
        Instruction inst = decode_instruction(linha.binary, linha.address);
        instrucoes.push_back(inst);

        if (inst.type == "INVALID") {
            cerr << "Instrucao invalida no endereco 0x" << hex << inst.address << dec << "\n";
        }
    }
}