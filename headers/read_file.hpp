#ifndef READFILE_HPP
#define READFILE_HPP

#include <string>
#include <vector>
#include "instruction.hpp"
using namespace std;

void read_file(string &input_file, vector<Instruction> &instruction);

struct LinhaLida {
    unsigned int address;
    string binary;
};

vector<LinhaLida> lerArquivoInstrucoes(const string &caminho, unsigned int enderecoBase); 
bool prepararLinha(const string &linhaOriginal, string &linhaLimpa);
bool ehFormatoBinario(const string &linha);
string converterParaBinario32(const string &linha, bool ehBinario);

#endif