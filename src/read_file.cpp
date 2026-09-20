#include "read_file.hpp"
#include <fstream>

bool prepararLinha(const string &linhaOriginal, string &linhaLimpa) {
    string linha = linhaOriginal;

    size_t inicio = linha.find_first_not_of(" \t\r\n");
    if (inicio == string::npos) return false; // linha vazia (só espaço/nada)
    size_t fim = linha.find_last_not_of(" \t\r\n");
    linha = linha.substr(inicio, fim - inicio + 1);

    if (linha.empty()) return false;

    // Comentário: linha começando com '#' ou '//'.
    if (linha[0] == '#') return false;
    if (linha.size() >= 2 && linha[0] == '/' && linha[1] == '/') return false;

    // Remove prefixo 0x/0X se presente.
    if (linha.size() >= 2 && linha[0] == '0' && (linha[1] == 'x' || linha[1] == 'X')) {
        linha = linha.substr(2);
    }

    linhaLimpa = linha;
    return true;
}
        
bool ehFormatoBinario(const string &linha) {
    for (char c : linha) {
        if (c != '0' && c != '1') return false;
    }

    return linha.size() > 8;
}

string converterParaBinario32(const string &linha, bool ehBinario) {
    string binary;

    if (ehBinario) {
        binary = linha;
    } else {
        unsigned long valor = stoul(linha, nullptr, 16);
        for (int i = 31; i >= 0; i--) {
            binary += ((valor >> i) & 1u) ? '1' : '0';
        }
        return binary;
    }

    while (binary.size() < 32) {
        binary = "0" + binary;
    }
    return binary;
}

vector<LinhaLida> lerArquivoInstrucoes(const string &caminho, unsigned int enderecoBase) {
    vector<LinhaLida> resultado;

    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo " << caminho << "\n";
        return resultado;
    }

    string linha;
    unsigned int endereco = enderecoBase;
    while (getline(arquivo, linha)) {
        string linhaLimpa;
        if (!prepararLinha(linha, linhaLimpa)) continue;

        bool binario = ehFormatoBinario(linhaLimpa);
        string binary32 = converterParaBinario32(linhaLimpa, binario);

        resultado.push_back({endereco, binary32});
        endereco += 4;
    }

    arquivo.close();
    return resultado;
}