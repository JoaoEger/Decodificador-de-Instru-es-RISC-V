#include "converter.hpp"
#include "report.hpp"
#include "cpi_calculator.hpp"

string formatarLinhaSaida(const Instruction &inst) {
    ostringstream out;
    out << toHex(inst.address, 8) << " | " << binaryToHex(inst.full_instruction)
        << " | " << left << setw(8) << inst.type << " | " << setw(8) << inst.mnemonic << " |";

    if (!inst.rd.empty()) out << " rd=" << bitsParaNumero(inst.rd);
    if (!inst.rs1.empty()) out << " rs1=" << bitsParaNumero(inst.rs1);
    if (!inst.rs2.empty()) out << " rs2=" << bitsParaNumero(inst.rs2);
    if (!inst.imm.empty()) out << " imm=" << bitsParaNumeroComSinal(inst.imm);
    if (!inst.opcode.empty()) out << " opcode=" << inst.opcode;
    if (!inst.function3.empty()) out << " function3=" << bitsParaNumero(inst.function3);
    if (!inst.function7.empty()) out << " function7=" << bitsParaNumero(inst.function7);

    out << "  ->  " << paraAssembly(inst);
    return out.str();
}

string relatorioCPI(const vector<Instruction> &instrucoes) {
    double cpiMedio = calcularCPIMedio(instrucoes);
    int totalInstrucoes = instrucoes.size();
    int totalInstR = 0, totalInstI = 0, totalInstS = 0, totalInstB = 0, totalInstU = 0, totalInstJ = 0;
    for (auto &inst : instrucoes) {
        if (inst.type == "R") totalInstR++;
        else if (inst.type == "I") totalInstI++;
        else if (inst.type == "S") totalInstS++;
        else if (inst.type == "B") totalInstB++;
        else if (inst.type == "U") totalInstU++;
        else if (inst.type == "J") totalInstJ++;
    }
    auto percentual = [totalInstrucoes](int quantidade) {
        double percentual = totalInstrucoes == 0 ? 0.0 : (100.0 * quantidade / totalInstrucoes);
        ostringstream out;
        out << fixed << setprecision(1) << percentual << "%";
        return out.str();
    };

    return "Total de Instruções: " + to_string(totalInstrucoes) + "\n" +
           "CPI Médio: " + to_string(cpiMedio) + "\n" +
           "Instruções R-type: " + to_string(totalInstR) + " (" + percentual(totalInstR) + ")\n" +
           "Instruções I-type: " + to_string(totalInstI) + " (" + percentual(totalInstI) + ")\n" +
           "Instruções S-type: " + to_string(totalInstS) + " (" + percentual(totalInstS) + ")\n" +
           "Instruções B-type: " + to_string(totalInstB) + " (" + percentual(totalInstB) + ")\n" +
           "Instruções U-type: " + to_string(totalInstU) + " (" + percentual(totalInstU) + ")\n" +
           "Instruções J-type: " + to_string(totalInstJ) + " (" + percentual(totalInstJ) + ")";
}