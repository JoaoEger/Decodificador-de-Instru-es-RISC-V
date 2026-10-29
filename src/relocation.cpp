#include "realocation.hpp"
#include "encoder.hpp"
#include "control.hpp"
#include "hazards.hpp"
#include "converter.hpp"
#include <algorithm>

static bool ehBranchOuJal(const Instruction &inst) {
    return inst.type == "B" || inst.type == "J";
}

struct Contexto {
    vector<BlocoBasico> blocos;
    vector<int> blocoDe;   // índice original -> número do bloco
    vector<int> alvoDe;    // índice original -> índice do alvo (-1: jalr ou fora do programa)
};

static Contexto montarContexto(const vector<Instruction> &instrucoes) {
    Contexto c;
    vector<Desvio> desvios = detectarDesvios(instrucoes);
    c.blocos = calcularBlocos(instrucoes, desvios);
    c.blocoDe.assign(instrucoes.size(), -1);
    for (size_t b = 0; b < c.blocos.size(); b++) {
        for (size_t i = c.blocos[b].inicio; i <= c.blocos[b].fim; i++) c.blocoDe[i] = (int)b;
    }
    c.alvoDe.assign(instrucoes.size(), -1);
    for (const Desvio &d : desvios) c.alvoDe[d.indice] = d.indiceAlvo;
    return c;
}

static int posicaoDaInstrucao(const vector<LinhaCorrigida> &programa, size_t indiceOriginal) {
    for (size_t p = 0; p < programa.size(); p++) {
        if (programa[p].indiceOriginal == (int)indiceOriginal) return (int)p;
    }
    return -1;
}

// Todo alvo de desvio é início de bloco, mas a reordenação pode ter tirado
// a instrução-alvo original do começo (ou mandado ela pro slot). O que vale
// é a primeira instrução que o bloco passou a ter no programa novo.
static int primeiraDoBloco(const vector<LinhaCorrigida> &programa, const BlocoBasico &bloco) {
    for (size_t p = 0; p < programa.size(); p++) {
        int i = programa[p].indiceOriginal;
        if (i >= 0 && (size_t)i >= bloco.inicio && (size_t)i <= bloco.fim) return (int)p;
    }
    return -1;
}

// Pra onde o desvio deve apontar: a primeira instrução do bloco, ou o
// começo da sequência de nops do caminho tomado que vem logo antes dela
// (só existe quando o caminho tomado precisa deles). Os nops de conflito
// comuns ficam de fora: quem salta pro alvo não precisa executá-los.
static int entradaDoBloco(const vector<LinhaCorrigida> &programa, const BlocoBasico &bloco) {
    int entrada = primeiraDoBloco(programa, bloco);
    if (entrada < 0) return -1;
    while (entrada > 0 && programa[entrada - 1].nopDoCaminho) entrada--;
    return entrada;
}

// Destino esperado de um desvio, como endereço no programa novo. Se o alvo
// original estava fora do programa, não mexeu de lugar: vale o endereço
// absoluto original.
static long destinoEsperado(const vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes,
                            const Contexto &c, unsigned base, size_t indiceOriginal) {
    int alvo = c.alvoDe[indiceOriginal];
    if (alvo >= 0) {
        int entrada = entradaDoBloco(programa, c.blocos[c.blocoDe[alvo]]);
        return (long)base + 4L * entrada;
    }
    const Instruction &orig = instrucoes[indiceOriginal];
    return (long)orig.address + bitsParaNumeroComSinal(orig.imm);
}

static int nopsDoCaminho(const vector<LinhaCorrigida> &programa, int posDesvio, int posAlvo) {
    const Instruction nop = criarNop("").inst;

    for (int x = 0; x <= 2; x++) {
        vector<const Instruction *> dinamica;
        dinamica.push_back(&programa[posDesvio].inst);
        dinamica.push_back(&programa[posDesvio + 1].inst);
        for (int k = 0; k < x; k++) dinamica.push_back(&nop);
        for (int p = posAlvo; p < (int)programa.size() && p < posAlvo + 4; p++) {
            dinamica.push_back(&programa[p].inst);
        }

        bool ok = true;
        for (size_t j = 2; j < dinamica.size() && ok; j++) {
            for (int reg : registradoresLidos(*dinamica[j])) {
                for (size_t d = 1; d <= 2 && d <= j; d++) {
                    if (registradorEscrito(*dinamica[j - d]) != reg) continue;
                    if (j - d <= 1) ok = false;   // o produtor mais recente é o desvio ou o slot
                    break;
                }
            }
        }
        if (ok) return x;
    }
    return 2;
}

// Garante, pra cada desvio com alvo conhecido, os nops que o caminho tomado
// exige. Reaproveita nops de conflito que já estão antes do alvo (só marca
// como "do caminho") e só cria nop novo quando eles não bastam.
void completarCaminhosTomados(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes) {
    if (instrucoes.empty()) return;
    Contexto c = montarContexto(instrucoes);

    for (size_t i = 0; i < instrucoes.size(); i++) {
        if (!ehDesvio(instrucoes[i])) continue;
        int alvo = c.alvoDe[i];
        if (alvo < 0) continue;

        int posDesvio = posicaoDaInstrucao(programa, i);
        int posAlvo = primeiraDoBloco(programa, c.blocos[c.blocoDe[alvo]]);
        if (posDesvio < 0 || posAlvo < 0) continue;
        if (posDesvio + 1 >= (int)programa.size()) continue;
        if (posAlvo == posDesvio || posAlvo == posDesvio + 1) continue;

        int necessarios = nopsDoCaminho(programa, posDesvio, posAlvo);

        int marcados = 0;
        while (posAlvo - 1 - marcados >= 0 && programa[posAlvo - 1 - marcados].nopDoCaminho) marcados++;
        int faltam = necessarios - marcados;

        int p = posAlvo - 1 - marcados;
        while (faltam > 0 && p >= 0 && programa[p].indiceOriginal == -1 && !programa[p].nopDoCaminho &&
               programa[p].marcacao.find("conflito de dados") != string::npos) {
            programa[p].nopDoCaminho = true;
            faltam--;
            p--;
        }

        while (faltam > 0) {
            LinhaCorrigida nop = criarNop("caminho tomado do desvio");
            nop.nopDoCaminho = true;
            programa.insert(programa.begin() + posAlvo, nop);
            posAlvo++;
            faltam--;
        }
    }
}

// R6: recálculo dos endereços e dos deslocamentos
void relocarDesvios(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes) {
    if (instrucoes.empty()) return;
    unsigned base = instrucoes[0].address;
    Contexto c = montarContexto(instrucoes);

    // 1. Endereço novo de cada linha (nops inclusos): é a posição no programa novo
    for (size_t k = 0; k < programa.size(); k++) {
        programa[k].inst.address = base + 4 * (unsigned)k;
    }

    // 2. Cada desvio original: novo_imm = destino novo - endereço novo do desvio
    for (size_t k = 0; k < programa.size(); k++) {
        int indice = programa[k].indiceOriginal;
        if (indice < 0) continue;
        const Instruction &orig = instrucoes[indice];
        if (!ehBranchOuJal(orig)) continue;

        long novoEndereco = (long)base + 4L * (long)k;
        long destino = destinoEsperado(programa, instrucoes, c, base, indice);
        long novoImm = destino - novoEndereco;

        if (!cabeNoDeslocamento(orig.type, novoImm)) {
            cerr << "Erro R6: o desvio em " << toHex(orig.address, 8) << " (agora " << toHex((unsigned)novoEndereco, 8)
                 << ") precisaria de deslocamento " << novoImm << ", que nao cabe no campo do tipo " << orig.type << "\n";
            continue;
        }

        string bits = codificarDeslocamento(programa[k].inst.full_instruction, orig.type, novoImm);
        programa[k].inst = decode_instruction(bits, (unsigned)novoEndereco);
    }
}

void finalizarPrograma(vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes) {
    completarCaminhosTomados(programa, instrucoes);
    relocarDesvios(programa, instrucoes);
}

int verificarRelocacao(const vector<LinhaCorrigida> &programa, const vector<Instruction> &instrucoes, int &conferidos) {
    conferidos = 0;
    int divergencias = 0;
    if (instrucoes.empty()) return 0;
    unsigned base = instrucoes[0].address;
    Contexto c = montarContexto(instrucoes);

    for (size_t k = 0; k < programa.size(); k++) {
        int indice = programa[k].indiceOriginal;
        if (indice < 0 || !ehBranchOuJal(instrucoes[indice])) continue;
        const Instruction &orig = instrucoes[indice];

        // Relê a palavra de máquina como o decodificador da M1 leria do arquivo
        string bits = programa[k].inst.full_instruction;
        Instruction lida = decode_instruction(bits, base + 4 * (unsigned)k);
        long destinoLido = (long)lida.address + bitsParaNumeroComSinal(lida.imm);
        long esperado = destinoEsperado(programa, instrucoes, c, base, indice);

        conferidos++;
        bool mesmosCampos = lida.mnemonic == orig.mnemonic && lida.rd == orig.rd &&
                            lida.rs1 == orig.rs1 && lida.rs2 == orig.rs2;
        if (destinoLido != esperado || !mesmosCampos) {
            divergencias++;
            cerr << "Divergencia R6: " << toHex(lida.address, 8) << " salta pra " << toHex((unsigned)destinoLido, 8)
                 << ", esperado " << toHex((unsigned)esperado, 8) << "\n";
        }
    }
    return divergencias;
}

void avisosDeRelocacao(const vector<Instruction> &instrucoes) {
    for (const Instruction &inst : instrucoes) {
        if (inst.mnemonic == "jalr") {
            cerr << "Aviso R6: jalr em " << toHex(inst.address, 8)
                 << " usa endereco em registrador; o alvo nao pode ser recalculado (limitacao)\n";
        }
        if (inst.mnemonic == "auipc") {
            cerr << "Aviso R6: auipc em " << toHex(inst.address, 8)
                 << " e relativo ao PC; o par auipc + addi/lw nao e recalculado (limitacao)\n";
        }
    }
}