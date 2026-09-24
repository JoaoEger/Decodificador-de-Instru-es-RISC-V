#include "converter.hpp"

static const array<string, 32> NOMES_ABI = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

// Uso geral (rd, rs1, rs2, funct3, funct7): campos pequenos, sempre positivos
int bitsParaNumero(const string &bits) {
    return stoi(bits, nullptr, 2);
}

// Uso exclusivo do imm: string de 32 bits que pode ter o bit de sinal ligado.
// Lemos como unsigned e reinterpretamos como int32_t (complemento de dois), se
// usar stoi/bitsParaNumero aqui estoura o range e lança exceção
long bitsParaNumeroComSinal(const string &bits) {
    unsigned long valor = stoul(bits, nullptr, 2);
    return static_cast<int32_t>(valor);
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

// Monta o assembly no formato convencional de cada tipo de instrução
string paraAssembly(const Instruction &inst) {
    if(inst.type == "R"){
        return inst.mnemonic + " "
            + nomeABI(bitsParaNumero(inst.rd)) + ", "
            + nomeABI(bitsParaNumero(inst.rs1)) + ", "
            + nomeABI(bitsParaNumero(inst.rs2));
    }
    if(inst.type == "I"){
        // loads e jalr acessam memória via rs1+imm, então usam a notação
        // "imm(rs1)" em vez de listar os operandos separados por vírgula
        bool usaFormatoOffset = inst.mnemonic == "lb" || inst.mnemonic == "lh" || inst.mnemonic == "lw"
            || inst.mnemonic == "lbu" || inst.mnemonic == "lhu" || inst.mnemonic == "jalr";

        // ecall/ebreak não têm operandos (não usam rd/rs1/imm de verdade)
        if (inst.mnemonic == "ecall" || inst.mnemonic == "ebreak") {
            return inst.mnemonic;
        }
        if (usaFormatoOffset) {
            return inst.mnemonic + " " + nomeABI(bitsParaNumero(inst.rd)) + ", "
                + to_string(bitsParaNumeroComSinal(inst.imm)) + "(" + nomeABI(bitsParaNumero(inst.rs1)) + ")";
        }

        return inst.mnemonic + " "
            + nomeABI(bitsParaNumero(inst.rd)) + ", "
            + nomeABI(bitsParaNumero(inst.rs1)) + ", "
            + to_string(bitsParaNumeroComSinal(inst.imm));
    }
    if(inst.type == "S"){
        return inst.mnemonic + " " + nomeABI(bitsParaNumero(inst.rs2)) + ", "
            + to_string(bitsParaNumeroComSinal(inst.imm)) + "(" + nomeABI(bitsParaNumero(inst.rs1)) + ")";
    }
    if(inst.type == "B"){
        // imm é um deslocamento relativo ao PC, não um endereço, ou seja, soma com
        // inst.address pra achar o destino real do desvio (ex: beq ..., 0x1000)
        long destino = bitsParaNumeroComSinal(inst.imm) + static_cast<long>(inst.address);
        return inst.mnemonic + " "
            + nomeABI(bitsParaNumero(inst.rs1)) + ", "
            + nomeABI(bitsParaNumero(inst.rs2)) + ", "
            + toHex(static_cast<unsigned int>(destino), 8);
    }
    if(inst.type == "U"){
        // Convenção de assembly: mostra o imediato de 20 bits cru (como está
        // na instrução), não o valor já deslocado 12 bits que fica em inst.imm.
        // full_instruction.substr(0, 20) pega direto os bits [31:12]
        return inst.mnemonic + " "
            + nomeABI(bitsParaNumero(inst.rd)) + ", "
            + toHex(bitsParaNumero(inst.full_instruction.substr(0, 20)), 5);
    }
    if(inst.type == "J"){
        long destino = bitsParaNumeroComSinal(inst.imm) + static_cast<long>(inst.address);
        return inst.mnemonic + " " + nomeABI(bitsParaNumero(inst.rd)) + ", "
            + toHex(static_cast<unsigned int>(destino), 8);
    }
    return "";
}