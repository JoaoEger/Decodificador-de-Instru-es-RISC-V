#include "hazards.hpp"
#include "converter.hpp"

// Registrador que a instrução escreve, ou -1 se não for produtora.
// Escrita em x0 é descartada pelo hardware (nop, jal x0, ret...), então
// não gera dependência. Formatos sem rd (S, B, INVALID) chegam com o campo vazio
int registradorEscrito(const Instruction &inst) {
    if (inst.rd.empty()) return -1;
    int rd = bitsParaNumero(inst.rd);
    return rd == 0 ? -1 : rd;
}

// Registradores que a instrução lê, sem o x0: ele sempre vale 0, então
// ler x0 nunca depende de instrução anterior. Testar empty() antes de
// converter evita a exceção do stoi em formatos sem rs1/rs2 (U, J, INVALID)
vector<int> registradoresLidos(const Instruction &inst) {
    vector<int> lidos;
    if (!inst.rs1.empty()) {
        int rs1 = bitsParaNumero(inst.rs1);
        if (rs1 != 0) lidos.push_back(rs1);
    }
    if (!inst.rs2.empty()) {
        int rs2 = bitsParaNumero(inst.rs2);
        if (rs2 != 0) lidos.push_back(rs2);
    }
    // ecall lê a7 (qual serviço) e a0 (argumento), mesmo sem esses
    // registradores aparecerem nos campos da instrução
    if (inst.mnemonic == "ecall") {
        lidos.push_back(17);
        lidos.push_back(10);
    }
    return lidos;
}

// Sem forwarding e com o banco escrito na 1ª metade do ciclo e lido na 2ª,
// só há conflito RAW quando o consumidor está a d = 1 ou d = 2 do produtor.
// Pra cada registrador lido, olha d=1 antes de d=2 e para no primeiro que
// bater: se as duas anteriores escrevem o mesmo registrador, vale a mais recente
vector<Hazard> detectarHazards(const vector<Instruction> &instrucoes) {
    vector<Hazard> hazards;
    for (size_t i = 0; i < instrucoes.size(); i++) {
        for (int reg : registradoresLidos(instrucoes[i])) {
            for (size_t d = 1; d <= 2 && d <= i; d++) {
                if (registradorEscrito(instrucoes[i - d]) == reg) {
                    hazards.push_back({i, i - d, reg, (int)d, 3 - (int)d});
                    break;
                }
            }
        }
    }
    return hazards;
}

// Formato do enunciado:
void imprimirHazards(const vector<Instruction> &instrucoes, const vector<Hazard> &hazards) {
    cout << "CONFLITOS DE DADOS\n";
    if (hazards.empty()) {
        cout << "  nenhum\n";
        return;
    }
    for (const Hazard &h : hazards) {
        const Instruction &consumidor = instrucoes[h.consumidor];
        const Instruction &produtor = instrucoes[h.produtor];
        cout << "  " << toHex(consumidor.address, 8) << "  " << left << setw(26) << paraAssembly(consumidor)
             << "<- " << toHex(produtor.address, 8) << "  " << setw(26) << paraAssembly(produtor)
             << "[" << nomeABI(h.registrador) << ", d=" << h.distancia << ", " << h.nops << " nops]\n";
    }
}
