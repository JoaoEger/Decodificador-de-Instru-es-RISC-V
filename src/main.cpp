#include "instruction.hpp"
#include "read_file.hpp"
#include "converter.hpp"
#include "report.hpp"
#include "hazards.hpp"
#include "control.hpp"
#include "correction.hpp"
#include "reorder.hpp"
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

    cout << string(130, '=') << "\n";
    cout << "Endereco   | Instrucao  | Tipo     | Mnemoni. | Registradores -> Assembly\n";
    cout << string(130, '=') << "\n";

    string caminhoInstrucoes = argv[1];
    vector<LinhaLida> linhas = lerArquivoInstrucoes(caminhoInstrucoes, enderecoInicial);
    
    vector<Instruction> instrucoes;
    instrucoes.reserve(linhas.size());
    for (LinhaLida &linha : linhas) {
        Instruction inst = decode_instruction(linha.binary, linha.address);
        instrucoes.push_back(inst);
    }

    for (const Instruction &inst : instrucoes) {
        cout << formatarLinhaSaida(inst) << "\n";
    }

    cout << string(130, '=') << "\n";
    cout << "\n";
    cout << relatorioCPI(instrucoes) << "\n\n";

    imprimirHazards(instrucoes, detectarHazards(instrucoes));
    cout << "\n";

    vector<Desvio> desvios = detectarDesvios(instrucoes);
    vector<BlocoBasico> blocos = calcularBlocos(instrucoes, desvios);
    vector<ConflitoFronteira> fronteira;
    for (const Desvio &d : desvios) {
        for (const ConflitoFronteira &c : conflitosNaFronteira(instrucoes, d)) fronteira.push_back(c);
    }
    imprimirConflitosControle(instrucoes, desvios, blocos, fronteira);
    cout << "\n";

    if (!instrucoes.empty()) {
        vector<LinhaCorrigida> somenteNops = corrigirComNops(instrucoes);
        cout << "PROGRAMA CORRIGIDO (SOMENTE NOPS)\n";
        imprimirProgramaCorrigido(somenteNops, instrucoes[0].address);

        // Conferência: rodando o R1 no programa novo não pode sobrar conflito
        vector<Instruction> novo;
        for (const LinhaCorrigida &linha : somenteNops) novo.push_back(linha.inst);
        cout << "  " << instrucoes.size() << " -> " << somenteNops.size() << " instrucoes, conflitos restantes: "
             << detectarHazards(novo).size() << "\n\n";

        vector<LinhaCorrigida> comReordenacao = reordenarComNops(instrucoes);
        cout << "PROGRAMA CORRIGIDO (NOPS + REORDENACAO)\n";
        imprimirProgramaCorrigido(comReordenacao, instrucoes[0].address);

        vector<Instruction> novoReordenado;
        for (const LinhaCorrigida &linha : comReordenacao) novoReordenado.push_back(linha.inst);
        cout << "  " << instrucoes.size() << " -> " << comReordenacao.size() << " instrucoes, conflitos restantes: "
             << detectarHazards(novoReordenado).size() << "\n\n";
    }

    cerr << "Instrucoes invalidas encontradas nos enderecos: ";
    for (const Instruction &inst : instrucoes) {
        // Cobre os dois jeitos de "não reconhecida", opcode desconhecido
        // (type == "INVALID") e opcode válido mas funct3/funct7 sem mnemônico mapeado.
        if (inst.type == "INVALID" || inst.mnemonic == "invalid") {
            cerr << toHex(inst.address, 8) << ", ";
        }
    }
}