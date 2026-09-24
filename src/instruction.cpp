#include "instruction.hpp"
#include "decode.hpp"

// Ponto de entrada da decodificação: descobre o formato pelo opcode e
// despacha pro extrator correspondente, que sabe onde estão os campos daquele formato
Instruction decode_instruction(string &binary, unsigned int address) {
    string opcode = findOpcode(binary);
    string type = identifyType(opcode);

    if (type == "R") return extract_R(binary, address);
    if (type == "I") return extract_I(binary, address);
    if (type == "S") return extract_S(binary, address);
    if (type == "B") return extract_B(binary, address);
    if (type == "U") return extract_U(binary, address);
    if (type == "J") return extract_J(binary, address);

    Instruction invalida;
    invalida.address = address;
    invalida.full_instruction = binary;
    invalida.type = "INVALID";
    invalida.mnemonic = "INVALID";
    invalida.opcode = opcode;
    return invalida;
}
