#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP
#include <iostream>
using namespace std;

struct Instruction {
    unsigned int address;                                    
    string full_instruction;
    string mnemonic;
    string type;

    string opcode;
    string function3;
    string function7;
    string rs1;
    string rs2;
    string rd;
    string imm;
};

Instruction decode_instruction(string &binary, unsigned int address);
string find_opcode(string &instruction);

#endif
