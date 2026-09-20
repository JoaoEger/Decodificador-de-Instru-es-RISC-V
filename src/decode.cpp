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

string identifyType(string &opcode) {
    if (opcode == "0110011") return "R";
    if (opcode == "0010011" || opcode == "0000011" || opcode == "1100111") return "I";
    if (opcode == "0100011") return "S";
    if (opcode == "1100011") return "B";
    if (opcode == "0110111" || opcode == "0010111") return "U";
    if (opcode == "1101111") return "J";

    return "INVALID";
}

string identify_mnemonic(string &binary, string &type) {
    string opcode = find_opcode(binary);
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