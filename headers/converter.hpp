#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

int bitsParaNumero(const string &bits);
long bitsParaNumeroComSinal(const string &bits);
string nomeABI(int registrador);

string toHex(unsigned int value, int width);
string binaryToHex(const string &binary);

string paraAssembly(const Instruction &inst);
string formatarLinhaSaida(const Instruction &inst);

#endif
