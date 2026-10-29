#include "output.hpp"
#include "read_file.hpp"
#include "converter.hpp"
#include <algorithm>
#include <fstream>

// Mesmo caminho da entrada, trocando a extensão pelo sufixo
// (ex: fibonacci_function.txt -> fibonacci_function_nops.txt)
string caminhoSaida(const string &entrada, const string &sufixo) {
    size_t ponto = entrada.find_last_of('.');
    size_t barra = entrada.find_last_of('/');
    bool temExtensao = ponto != string::npos && (barra == string::npos || ponto > barra);
    return (temExtensao ? entrada.substr(0, ponto) : entrada) + sufixo;
}

// Olha a primeira instrução do arquivo de entrada pra gravar a saída no
// mesmo formato (binário ou hexadecimal)
bool entradaEmBinario(const string &caminho) {
    ifstream arquivo(caminho);
    string linha, limpa;
    while (getline(arquivo, linha)) {
        if (prepararLinha(linha, limpa)) return ehFormatoBinario(limpa);
    }
    return false;
}

// Uma instrução por linha. O comentário no topo é ignorado pelo leitor da M1
bool salvarPrograma(const string &caminho, const vector<LinhaCorrigida> &programa, bool binario, const string &titulo) {
    ofstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel criar o arquivo " << caminho << "\n";
        return false;
    }
    arquivo << "// " << titulo << "\n";
    for (const LinhaCorrigida &linha : programa) {
        if (binario) arquivo << linha.inst.full_instruction << "\n";
        else arquivo << binaryToHex(linha.inst.full_instruction) << "\n";
    }
    return true;
}

bool salvarListagem(const string &caminho, const vector<LinhaCorrigida> &programa, unsigned int enderecoInicial,
                    const string &titulo) {
    ofstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel criar o arquivo " << caminho << "\n";
        return false;
    }
    arquivo << titulo << "\n";
    arquivo << "  Endereco    Codigo      Instrucao                   Marcacao\n";
    imprimirProgramaCorrigido(arquivo, programa, enderecoInicial);
    return true;
}

// Percentual com vírgula, igual ao exemplo do enunciado (+75,0%)
static string percentual(double valor) {
    ostringstream out;
    out << fixed << setprecision(1) << valor << "%";
    string texto = out.str();
    replace(texto.begin(), texto.end(), '.', ',');
    return texto;
}

// "nome ......" sempre com a mesma largura, pra alinhar os números
static string rotulo(const string &nome) {
    return nome + " " + string(22 - nome.size(), '.') + " ";
}

static void linhaCusto(const string &nome, size_t total, size_t original) {
    size_t acrescimo = total - original;
    cout << rotulo(nome) << total << " instrucoes (+" << acrescimo << ", +"
         << percentual(100.0 * acrescimo / original) << ")\n";
}

void imprimirCusto(size_t original, size_t somenteNops, size_t comReordenacao) {
    cout << "CUSTO\n";
    cout << rotulo("original") << original << " instrucoes\n";
    linhaCusto("somente nops", somenteNops, original);
    linhaCusto("nops + reordenacao", comReordenacao, original);

    // Comparação: quanto a reordenação economiza em relação a usar só nops
    long economia = (long)somenteNops - (long)comReordenacao;
    cout << rotulo("comparacao") << "a reordenacao usa " << economia << " instrucoes a menos ("
         << percentual(somenteNops == 0 ? 0.0 : 100.0 * economia / somenteNops) << " menor que somente nops)\n";
}
