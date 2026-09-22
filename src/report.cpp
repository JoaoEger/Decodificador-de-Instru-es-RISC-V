#include "converter.hpp"
#include "report.hpp"

string formatarLinhaSaida(const Instruction &inst) {
    ostringstream out;
    out << toHex(inst.address, 8) << " | " << binaryToHex(inst.full_instruction)
        << " | " << left << setw(8) << inst.type << " | " << setw(8) << inst.mnemonic << " |";

    if (!inst.rd.empty()) out << " rd=" << bitsParaNumero(inst.rd);
    if (!inst.rs1.empty()) out << " rs1=" << bitsParaNumero(inst.rs1);
    if (!inst.rs2.empty()) out << " rs2=" << bitsParaNumero(inst.rs2);

    out << "  ->  " << paraAssembly(inst);
    return out.str();
}

string relatorioCPI(const vector<Instruction> &instrucoes) {
    //a fazer
    return "";
}