#include "instruction.hpp"
#include "read_file.hpp"
#include "converter.hpp"
#include "report.hpp"
#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Execute o programa passando como parametro o caminho do arquivo em que esta as instrucoes!\n";
        return 1;
    }
    string enderecoInicial;
    cout << "Adicione o endereco inicial das instrucoes (em hexadecimal, apenas os digitos apos 0x Ex: Certo: 80AFD000 | Errado: 0x80AFD000): ";
    cin >> enderecoInicial;
    while(enderecoInicial.empty() || ehFormatoHexadecimal(enderecoInicial) == false){
        if(enderecoInicial.empty()) {
            cout << "Endereco inicial vazio! Digite novamente (8 digitos): ";
        } else if(ehFormatoHexadecimal(enderecoInicial) == false) {
            cout << "Endereco inicial invalido! Digite apenas os digitos em hexadecimal (apenas os digitos apos 0x Ex: Certo: 80AFD000 | Errado: 0x80AFD000): ";
        }
        cin >> enderecoInicial;
    };
    cout << string(85, '=') << "\n";
    cout << "Endereco   | Instrucao  | Tipo     | Mnemoni. | Registradores -> Assembly\n";
    cout << string(85, '=') << "\n";

    string caminhoInstrucoes = argv[1];
    vector<LinhaLida> linhas = lerArquivoInstrucoes(caminhoInstrucoes, enderecoInicial);
    
    vector<Instruction> instrucoes;
    instrucoes.reserve(linhas.size());
    for (LinhaLida &linha : linhas) {
        Instruction inst = decode_instruction(linha.binary, linha.address);
        instrucoes.push_back(inst);

        if (inst.type == "INVALID") {
            cerr << "Instrucao invalida no endereco 0x" << hex << inst.address << dec << "\n";
        }
    }

    for (const Instruction &inst : instrucoes) {
        cout << formatarLinhaSaida(inst) << "\n";
    }

    cout << relatorioCPI(instrucoes) << "\n";
}