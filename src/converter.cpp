#include "converter.hpp"

static const array<string, 32> NOMES_ABI = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

int bitsParaNumero(const string &bits) {
    return stoi(bits, nullptr, 2);
}

string nomeABI(int registrador) {
    if (registrador < 0 || registrador > 31) return "x?";
    return NOMES_ABI[registrador];
}

string toHex(unsigned int value, int width) {
    ostringstream out;
    out << "0x" << hex << setfill('0') << setw(width) << value;
    return out.str();
}

string binaryToHex(const string &binary) {
    unsigned long valor = stoul(binary, nullptr, 2);
    return toHex(static_cast<unsigned int>(valor), 8);
}

string paraAssembly(const Instruction &inst) {
    return inst.mnemonic;
    // a fazer, return só pra testar
}