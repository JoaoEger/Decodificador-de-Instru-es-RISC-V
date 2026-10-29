#include "encoder.hpp"
#include <cstdint>

bool cabeNoDeslocamento(const string &tipo, long deslocamento) {
    if (deslocamento % 2 != 0) return false;
    if (tipo == "B") return deslocamento >= -4096 && deslocamento <= 4094;
    if (tipo == "J") return deslocamento >= -1048576 && deslocamento <= 1048574;
    return false;
}

// Os mesmos bits que decode.cpp junta na leitura (extract_B / extract_J),
// só que no caminho inverso: espalha o imediato nas posições embaralhadas
// Os outros campos (opcode, rd, rs1, rs2, funct3) ficam como estavam.
string codificarDeslocamento(const string &bits, const string &tipo, long deslocamento) {
    uint32_t palavra = static_cast<uint32_t>(stoul(bits, nullptr, 2));
    uint32_t imm = static_cast<uint32_t>(deslocamento);

    if (tipo == "B") {
        palavra &= 0x01FFF07F; // mantém opcode, funct3, rs1 e rs2
        palavra |= ((imm >> 12) & 0x1u)  << 31;
        palavra |= ((imm >> 5)  & 0x3Fu) << 25;
        palavra |= ((imm >> 1)  & 0xFu)  << 8;
        palavra |= ((imm >> 11) & 0x1u)  << 7;
    } else if (tipo == "J") {
        palavra &= 0x00000FFF; // mantém opcode e rd
        palavra |= ((imm >> 20) & 0x1u)   << 31;
        palavra |= ((imm >> 1)  & 0x3FFu) << 21;
        palavra |= ((imm >> 11) & 0x1u)   << 20;
        palavra |= ((imm >> 12) & 0xFFu)  << 12;
    }

    string saida(32, '0');
    for (int i = 0; i < 32; i++) {
        if ((palavra >> i) & 1u) saida[31 - i] = '1';
    }
    return saida;
}