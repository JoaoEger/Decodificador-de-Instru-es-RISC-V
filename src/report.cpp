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

    string pseudo = identificarPseudoInstrucao(inst);
    if (!pseudo.empty()) out << "  (pseudo: " << pseudo << ")";

    return out.str();
}
string identificarPseudoInstrucao(const Instruction &inst) {
    // addi x0, x0, 0 = nop
    if (inst.mnemonic == "addi" && bitsParaNumero(inst.rd) == 0 && bitsParaNumero(inst.rs1) == 0 && bitsParaNumeroComSinal(inst.imm) == 0) {
        return "nop";
    }
    // jalr x0, 0(x1) = ret
    if (inst.mnemonic == "jalr" && bitsParaNumero(inst.rd) == 0 && bitsParaNumero(inst.rs1) == 1 && bitsParaNumeroComSinal(inst.imm) == 0) {
        return "ret";
    }
    // addi x1, x0, 0 = mv x1, x0
    if (inst.mnemonic == "addi" && bitsParaNumeroComSinal(inst.imm) == 0) {
        return "mv";
    }
    // xori x1, x0, -1 = not x1, x0
    if (inst.mnemonic == "xori" && bitsParaNumeroComSinal(inst.imm) == -1) {
        return "not";
    }
    return "";
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

    return "Total de Instrucoes: " + to_string(totalInstrucoes) + "\n" +
           "CPI Medio: " + to_string(cpiMedio) + "\n" +
           "Instrucoes R-type: " + to_string(totalInstR) + " (" + percentual(totalInstR) + ")\n" +
           "Instrucoes I-type: " + to_string(totalInstI) + " (" + percentual(totalInstI) + ")\n" +
           "Instrucoes S-type: " + to_string(totalInstS) + " (" + percentual(totalInstS) + ")\n" +
           "Instrucoes B-type: " + to_string(totalInstB) + " (" + percentual(totalInstB) + ")\n" +
           "Instrucoes U-type: " + to_string(totalInstU) + " (" + percentual(totalInstU) + ")\n" +
           "Instrucoes J-type: " + to_string(totalInstJ) + " (" + percentual(totalInstJ) + ")";
}