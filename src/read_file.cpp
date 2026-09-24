#include "read_file.hpp"
#include <fstream>

bool prepararLinha(const string &linhaOriginal, string &linhaLimpa) {
    string linha = linhaOriginal;

    size_t inicio = linha.find_first_not_of(" \t\r\n");
    if (inicio == string::npos) return false; // linha vazia (só espaço/nada)
    size_t fim = linha.find_last_not_of(" \t\r\n");
    linha = linha.substr(inicio, fim - inicio + 1);

    if (linha.empty()) return false;

    // Comentário: linha começando com '#' ou '//'
    if (linha[0] == '#') return false;
    if (linha.size() >= 2 && linha[0] == '/' && linha[1] == '/') return false;

    // Remove prefixo 0x/0X se presente
    if (linha.size() >= 2 && linha[0] == '0' && (linha[1] == 'x' || linha[1] == 'X')) {
        linha = linha.substr(2);
    }

    linhaLimpa = linha;
    return true;
}
        
// Um hex de 8 dígitos também pode ser só '0'/'1' (ex: "00110011"), então
// o size() > 8 é o que desempata: binário de 32 bits sempre tem mais de 8 chars.
bool ehFormatoBinario(const string &linha) {
    for (char c : linha) {
        if (c != '0' && c != '1') return false;
    }

    return linha.size() > 8;
}

bool ehFormatoHexadecimal(const string &linha) {
    for (char c : linha) {
        if (!isxdigit(c)) return false;
    }

    return linha.size() <= 8;
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

vector<LinhaLida> lerArquivoInstrucoes(const string &caminho, string enderecoInicial) {
    vector<LinhaLida> resultado;

    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo " << caminho << "\n";
        return resultado;
    }

    string linha;
    int numeroLinha = 0;
    unsigned int endereco = stoul(enderecoInicial, nullptr, 16); // R1: endereço-base configurável
    while (getline(arquivo, linha)) {
        numeroLinha++;
        string linhaLimpa;
        if (!prepararLinha(linha, linhaLimpa)) continue;

        // Valida antes de converter: sem isso, qualquer linha que não fosse binária
        // ia direto pro stoul como hex e um caractere inválido derrubava o programa
        // Binário com mais de 32 bits e "0x" sozinho (linha vazia) também são rejeitados
        bool binario = ehFormatoBinario(linhaLimpa) && linhaLimpa.size() <= 32;
        bool hexadecimal = !linhaLimpa.empty() && ehFormatoHexadecimal(linhaLimpa);
        if (!binario && !hexadecimal) {
            cerr << "Aviso: linha " << numeroLinha << " ignorada, formato invalido: \""
                 << linhaLimpa << "\"\n";
            continue;
        }

        string binary32 = converterParaBinario32(linhaLimpa, binario);

        resultado.push_back({endereco, binary32});
        endereco += 4;
    }

    arquivo.close();
    return resultado;
}