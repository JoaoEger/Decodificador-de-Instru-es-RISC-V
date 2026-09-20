#ifndef DECODE_HPP
#define DECODE_HPP

#include <string>
#include "instruction.hpp"
using namespace std;

string identifyType(string &opcode);
string identifyMnemonic(string &binary, string &type);

string extractBits(const string &binary, int bitAlto, int bitBaixo);

Instruction extract_R(string &binary, unsigned int address);
Instruction extract_I(string &binary, unsigned int address);
Instruction extract_S(string &binary, unsigned int address);
Instruction extract_B(string &binary, unsigned int address);
Instruction extract_U(string &binary, unsigned int address);
Instruction extract_J(string &binary, unsigned int address);

#endif