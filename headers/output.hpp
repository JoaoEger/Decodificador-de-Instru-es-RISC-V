#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "correction.hpp"
#include <string>
#include <vector>
using namespace std;

string caminhoSaida(const string &entrada, const string &sufixo);
bool entradaEmBinario(const string &caminho);

// R7: programa no mesmo formato da entrada (lido de volta pelo decodificador)
// e listagem legível com endereço, código, assembly e marcação
bool salvarPrograma(const string &caminho, const vector<LinhaCorrigida> &programa, bool binario, const string &titulo);
bool salvarListagem(const string &caminho, const vector<LinhaCorrigida> &programa, unsigned int enderecoInicial,
                    const string &titulo);

// R8: custo de cada técnica em relação ao original
void imprimirCusto(size_t original, size_t somenteNops, size_t comReordenacao);

#endif
