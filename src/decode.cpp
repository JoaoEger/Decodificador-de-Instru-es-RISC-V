#include "decode.hpp"

// Extrai um intervalo de bits [bitAlto:bitBaixo] de uma string
string extractBits(const string &binary, int bitAlto, int bitBaixo) {
    int indiceInicio = 31 - bitAlto;
    int quantidade = bitAlto - bitBaixo + 1;
    return binary.substr(indiceInicio, quantidade);
}

string findOpcode(string &instruction) {
    return extractBits(instruction, 6, 0);
}

// Extende o sinal de uma string binária até 32 bits
static string signExtend(const string &bits) {
    char bitDeSinal = bits[0];
    return string(32 - bits.size(), bitDeSinal) + bits;
}

string identifyType(string &opcode) {
    if (opcode == "0110011") return "R";
    if (opcode == "0010011" || opcode == "0000011" || opcode == "1100111" || opcode == "1110011") return "I";
    if (opcode == "0100011") return "S";
    if (opcode == "1100011") return "B";
    if (opcode == "0110111" || opcode == "0010111") return "U";
    if (opcode == "1101111") return "J";

    return "INVALID";
}

string identifyMnemonic(string &binary, string &type) {
    string opcode = findOpcode(binary);
    string funct3 = extractBits(binary, 14, 12);
    string funct7 = extractBits(binary, 31, 25);

    if (type == "R") {
        if (funct3 == "000") return funct7 == "0100000" ? "sub" : "add";
        if (funct3 == "001") return "sll";
        if (funct3 == "010") return "slt";
        if (funct3 == "011") return "sltu";
        if (funct3 == "100") return "xor";
        if (funct3 == "101") return funct7 == "0100000" ? "sra" : "srl";
        if (funct3 == "110") return "or";
        if (funct3 == "111") return "and";

        return "invalid";
    }

    if (type == "I") {
        if (opcode == "0010011") {
            if (funct3 == "000") return "addi";
            if (funct3 == "010") return "slti";
            if (funct3 == "011") return "sltiu";
            if (funct3 == "100") return "xori";
            if (funct3 == "110") return "ori";
            if (funct3 == "111") return "andi";
            if (funct3 == "001") return "slli";
            if (funct3 == "101") return funct7 == "0100000" ? "srai" : "srli";

            return "invalid";
        }
        if (opcode == "0000011") {
            if (funct3 == "000") return "lb";
            if (funct3 == "001") return "lh";
            if (funct3 == "010") return "lw";
            if (funct3 == "100") return "lbu";
            if (funct3 == "101") return "lhu";

            return "invalid";
        }
        if (opcode == "1100111") return "jalr";

        // ecall/ebreak usam o mesmo opcode de I mas não têm rd/rs1/imm de
        // verdade, os 12 bits do "imm" só servem pra distinguir os dois
        if (opcode == "1110011") {
            string immBits = extractBits(binary, 31, 20);
            if (immBits == "000000000000") return "ecall";
            if (immBits == "000000000001") return "ebreak";
            return "invalid";
        }

        return "invalid";
    }

    if (type == "S") {
        if (funct3 == "000") return "sb";
        if (funct3 == "001") return "sh";
        if (funct3 == "010") return "sw";

        return "invalid";
    }

    if (type == "B") {
        if (funct3 == "000") return "beq";
        if (funct3 == "001") return "bne";
        if (funct3 == "100") return "blt";
        if (funct3 == "101") return "bge";
        if (funct3 == "110") return "bltu";
        if (funct3 == "111") return "bgeu";

        return "invalid";
    }

    if (type == "U") return opcode == "0110111" ? "lui" : "auipc";
    if (type == "J") return "jal";

    return "invalid";
}

// Monta os campos comuns a qualquer tipo de instrução; o tipo é
// descoberto a partir do opcode, não fixado por quem chama
static Instruction baseInstruction(string &binary, unsigned int address) {
    Instruction inst;
    inst.address = address;
    inst.full_instruction = binary;
    inst.opcode = findOpcode(binary);
    inst.type = identifyType(inst.opcode);
    inst.function3 = "";
    inst.function7 = "";
    inst.rd = "";
    inst.rs1 = "";
    inst.rs2 = "";
    inst.imm = "";
    return inst;
}

static Instruction finish(Instruction inst, string &binary) {
    inst.mnemonic = identifyMnemonic(binary, inst.type);
    return inst;
}

Instruction extract_R(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.function3 = extractBits(binary, 14, 12);
    inst.function7 = extractBits(binary, 31, 25);
    inst.rd = extractBits(binary, 11, 7);
    inst.rs1 = extractBits(binary, 19, 15);
    inst.rs2 = extractBits(binary, 24, 20);
    return finish(inst, binary);
}

Instruction extract_I(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.function3 = extractBits(binary, 14, 12);
    inst.rd = extractBits(binary, 11, 7);
    inst.rs1 = extractBits(binary, 19, 15);
    inst.imm = signExtend(extractBits(binary, 31, 20));
    return finish(inst, binary);
}

Instruction extract_S(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.function3 = extractBits(binary, 14, 12);
    inst.rs1 = extractBits(binary, 19, 15);
    inst.rs2 = extractBits(binary, 24, 20);
    inst.imm = signExtend(extractBits(binary, 31, 25) + extractBits(binary, 11, 7));
    return finish(inst, binary);
}

Instruction extract_B(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.function3 = extractBits(binary, 14, 12);
    inst.rs1 = extractBits(binary, 19, 15);
    inst.rs2 = extractBits(binary, 24, 20);

    // Bits do imediato vêm fora de ordem no formato B (economiza espaço
    // mantendo o bit 0 dos formatos R/I/S/U na mesma posição). O "0" final
    // é o bit menos significativo, sempre 0, pois desvios são alinhados em 2 bytes
    string bit12 = extractBits(binary, 31, 31);
    string bit11 = extractBits(binary, 7, 7);
    string bits10_5 = extractBits(binary, 30, 25);
    string bits4_1 = extractBits(binary, 11, 8);
    inst.imm = signExtend(bit12 + bit11 + bits10_5 + bits4_1 + "0");
    return finish(inst, binary);
}

Instruction extract_U(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.rd = extractBits(binary, 11, 7);
    // imm[31:12] já ocupa os 20 bits mais altos; os 12 bits baixos são
    // sempre 0 por definição do formato (lui/auipc carregam só a parte alta)
    inst.imm = extractBits(binary, 31, 12) + string(12, '0');
    return finish(inst, binary);
}

Instruction extract_J(string &binary, unsigned int address) {
    Instruction inst = baseInstruction(binary, address);
    inst.rd = extractBits(binary, 11, 7);

    // Mesma ideia do formato B: bits embaralhados + bit 0 implícito (sempre 0)
    string bit20 = extractBits(binary, 31, 31);
    string bits19_12 = extractBits(binary, 19, 12);
    string bit11 = extractBits(binary, 20, 20);
    string bits10_1 = extractBits(binary, 30, 21);
    inst.imm = signExtend(bit20 + bits19_12 + bit11 + bits10_1 + "0");
    return finish(inst, binary);
}