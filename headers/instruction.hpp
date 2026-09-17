#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP
#include <iostream> 
using namespace std;

struct Instruction {
    string full_instruction;
    string rs1;
    string rs2;
    string rd;
    string opcode;
    string function3;
    string function7;
    string imm;
    string type;
};

Instruction decode_instruction(string &binary);
string find_opcode(string &instructions);

#endif