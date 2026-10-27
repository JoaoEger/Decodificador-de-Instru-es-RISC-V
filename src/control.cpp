#include "control.hpp"
#include "converter.hpp"

// Branches (B), jal (J) e jalr mudam o PC. No pipeline sem flush, todos
// deixam a instrução seguinte (o slot de atraso) executar
bool ehDesvio(const Instruction &inst) {
    if (inst.type == "B") return true;
    if (inst.type == "J") return true;
    if (inst.mnemonic == "jalr") return true;
    return false;
}

// Converte o destino do desvio numa posição do vetor de instruções.
// O imm do branch/jal é a distância até o alvo, então alvo = endereço + imm.
// Cada instrução tem 4 bytes: a posição é (alvo - endereço da 1ª) / 4.
// Retorna -1 pro jalr (alvo vem de registrador) ou se o alvo cair fora do programa
static int calcularIndiceAlvo(const vector<Instruction> &instrucoes, const Instruction &desvio) {
    if (desvio.mnemonic == "jalr") return -1;

    long alvo = (long)desvio.address + bitsParaNumeroComSinal(desvio.imm);
    long primeiro = instrucoes[0].address;
    long indice = (alvo - primeiro) / 4;

    if (indice < 0 || indice >= (long)instrucoes.size()) return -1;
    return (int)indice;
}

vector<Desvio> detectarDesvios(const vector<Instruction> &instrucoes) {
    vector<Desvio> desvios;
    for (size_t i = 0; i < instrucoes.size(); i++) {
        if (!ehDesvio(instrucoes[i])) continue;

        Desvio d;
        d.indice = i;
        d.condicional = instrucoes[i].type == "B";
        d.indiceAlvo = calcularIndiceAlvo(instrucoes, instrucoes[i]);
        desvios.push_back(d);
    }
    return desvios;
}

// Primeiro marca quem COMEÇA um bloco (os "líderes"):
//   - a primeira instrução do programa
//   - todo alvo de desvio (alguém pode pular pra lá)
//   - a instrução logo depois de um desvio (o desvio fecha o bloco)
// Depois corta o programa em pedaços: cada líder abre um bloco novo
vector<BlocoBasico> calcularBlocos(const vector<Instruction> &instrucoes, const vector<Desvio> &desvios) {
    vector<BlocoBasico> blocos;
    size_t n = instrucoes.size();
    if (n == 0) return blocos;

    vector<bool> ehLider(n, false);
    ehLider[0] = true;
    for (const Desvio &d : desvios) {
        if (d.indiceAlvo != -1) ehLider[d.indiceAlvo] = true;
        if (d.indice + 1 < n) ehLider[d.indice + 1] = true;
    }

    BlocoBasico atual;
    atual.inicio = 0;
    for (size_t i = 1; i < n; i++) {
        if (ehLider[i]) {
            atual.fim = i - 1;         // fecha o bloco anterior
            blocos.push_back(atual);
            atual.inicio = i;          // abre um novo
        }
    }
    atual.fim = n - 1;                 // fecha o último
    blocos.push_back(atual);
    return blocos;
}

// Ao entrar num caminho, a ordem real de execução é:
//
//     desvio -> slot -> primeira do caminho -> segunda do caminho
//
// Então as distâncias são:
//     slot   -> primeira  : d = 1  (2 nops)
//     desvio -> primeira  : d = 2  (1 nop)   só importa pro jal, branch não escreve
//     slot   -> segunda   : d = 2  (1 nop)
// A instrução antes do desvio fica a d = 3 da primeira: sem conflito

static bool escreve(const Instruction &inst, int reg) {
    return registradorEscrito(inst) == reg;
}

static void adicionar(vector<ConflitoFronteira> &saida, const Desvio &desvio, bool tomado,
                      size_t consumidor, size_t produtor, int reg, int d) {
    ConflitoFronteira c;
    c.desvio = desvio.indice;
    c.caminhoTomado = tomado;
    c.hazard = {consumidor, produtor, reg, d, 3 - d};
    saida.push_back(c);
}

// Confere as duas primeiras instruções de um caminho que começa em "inicio"
static void verificarCaminho(const vector<Instruction> &instrucoes, const Desvio &desvio,
                             size_t inicio, bool tomado, vector<ConflitoFronteira> &saida) {
    size_t indiceDesvio = desvio.indice;
    size_t indiceSlot = desvio.indice + 1;
    const Instruction &instDesvio = instrucoes[indiceDesvio];
    const Instruction &slot = instrucoes[indiceSlot];

    // Primeira instrução do caminho: o produtor mais próximo é o slot (d=1),
    // depois o próprio desvio (d=2). Se o slot escreve, ele é o mais recente
    size_t primeira = inicio;
    for (int reg : registradoresLidos(instrucoes[primeira])) {
        if (escreve(slot, reg)) {
            adicionar(saida, desvio, tomado, primeira, indiceSlot, reg, 1);
        } else if (escreve(instDesvio, reg)) {
            adicionar(saida, desvio, tomado, primeira, indiceDesvio, reg, 2);
        }
    }

    // Segunda instrução do caminho: o produtor mais próximo é a primeira (d=1),
    // mas essa dupla está dentro do caminho e já é conflito do R1. Se a
    // primeira não escreve o registrador, olha o slot (d=2)
    size_t segunda = inicio + 1;
    if (segunda >= instrucoes.size()) return;
    for (int reg : registradoresLidos(instrucoes[segunda])) {
        if (escreve(instrucoes[primeira], reg)) continue;
        if (escreve(slot, reg)) {
            adicionar(saida, desvio, tomado, segunda, indiceSlot, reg, 2);
        }
    }
}

// Caminho não tomado: só existe no branch condicional, começa depois do slot.
// Caminho tomado: começa no alvo, se ele for conhecido e estiver no programa
vector<ConflitoFronteira> conflitosNaFronteira(const vector<Instruction> &instrucoes, const Desvio &desvio) {
    vector<ConflitoFronteira> conflitos;

    bool temSlot = desvio.indice + 1 < instrucoes.size();
    if (!temSlot) return conflitos;

    size_t depoisDoSlot = desvio.indice + 2;
    if (desvio.condicional && depoisDoSlot < instrucoes.size()) {
        verificarCaminho(instrucoes, desvio, depoisDoSlot, false, conflitos);
    }
    if (desvio.indiceAlvo != -1) {
        verificarCaminho(instrucoes, desvio, desvio.indiceAlvo, true, conflitos);
    }
    return conflitos;
}

// ============================================================================
// Impressão
// ============================================================================

void imprimirConflitosControle(const vector<Instruction> &instrucoes, const vector<Desvio> &desvios,
                               const vector<BlocoBasico> &blocos, const vector<ConflitoFronteira> &fronteira) {
    cout << "CONFLITOS DE CONTROLE\n";
    if (desvios.empty()) cout << "  nenhum\n";
    for (const Desvio &d : desvios) {
        const Instruction &inst = instrucoes[d.indice];
        cout << "  " << toHex(inst.address, 8) << "  " << left << setw(26) << paraAssembly(inst);

        if (d.indice + 1 < instrucoes.size()) {
            const Instruction &slot = instrucoes[d.indice + 1];
            cout << "slot ocupado por: " << toHex(slot.address, 8) << "  " << paraAssembly(slot);
        } else {
            cout << "sem slot (ultima instrucao do programa)";
        }

        if (inst.mnemonic == "jalr") cout << "  [alvo dinamico]";
        else if (d.indiceAlvo == -1) cout << "  [alvo fora do programa]";
        cout << "\n";
    }

    cout << "\nBLOCOS BASICOS\n";
    for (size_t b = 0; b < blocos.size(); b++) {
        size_t tamanho = blocos[b].fim - blocos[b].inicio + 1;
        cout << "  B" << left << setw(3) << b
             << toHex(instrucoes[blocos[b].inicio].address, 8) << " - "
             << toHex(instrucoes[blocos[b].fim].address, 8)
             << "  (" << tamanho << " instrucoes)\n";
    }

    cout << "\nCONFLITOS NA FRONTEIRA DO DESVIO\n";
    if (fronteira.empty()) cout << "  nenhum\n";
    for (const ConflitoFronteira &c : fronteira) {
        const Instruction &consumidor = instrucoes[c.hazard.consumidor];
        const Instruction &produtor = instrucoes[c.hazard.produtor];
        cout << "  " << toHex(consumidor.address, 8) << "  " << left << setw(26) << paraAssembly(consumidor)
             << "<- " << toHex(produtor.address, 8) << "  " << setw(26) << paraAssembly(produtor)
             << "[" << nomeABI(c.hazard.registrador) << ", d=" << c.hazard.distancia << ", "
             << c.hazard.nops << " nops, " << (c.caminhoTomado ? "tomado" : "nao tomado")
             << ", desvio em " << toHex(instrucoes[c.desvio].address, 8) << "]\n";
    }
}
