#include "reorder.hpp"
#include "hazards.hpp"
#include "converter.hpp"
#include <algorithm>
#include <climits>
using namespace std;

bool leDaMemoria(const Instruction &inst) {
    return inst.mnemonic == "lb" || inst.mnemonic == "lh" || inst.mnemonic == "lw" ||
           inst.mnemonic == "lbu" || inst.mnemonic == "lhu";
}

bool escreveNaMemoria(const Instruction &inst) {
    return inst.type == "S";
}

bool acessaMemoria(const Instruction &inst) {
    return leDaMemoria(inst) || escreveNaMemoria(inst);
}

static int baseMemoria(const Instruction &inst) {
    return bitsParaNumero(inst.rs1);
}

static long deslocamentoMemoria(const Instruction &inst) {
    return bitsParaNumeroComSinal(inst.imm);
}

static bool baseFoiReescrita(const vector<Instruction> &instrucoes, int registrador, size_t de, size_t para) {
    for (size_t k = de + 1; k < para; k++) {
        if (registradorEscrito(instrucoes[k]) == registrador) return true;
    }
    return false;
}

// bases diferentes = assume que podem se sobrepor (sem info
// suficiente pra garantir o contrario); mesma base + mesmo deslocamento
// = mesmo endereco; mesma base + deslocamento diferente = enderecos
// distintos, DESDE que a base nao tenha sido reescrita entre as duas.
static bool podemSobrepor(const vector<Instruction> &instrucoes, size_t i, size_t j) {
    const Instruction &a = instrucoes[i];
    const Instruction &b = instrucoes[j];

    int baseA = baseMemoria(a);
    int baseB = baseMemoria(b);
    if (baseA != baseB) return true;

    if (baseFoiReescrita(instrucoes, baseA, i, j)) return true;

    return deslocamentoMemoria(a) == deslocamentoMemoria(b);
}

vector<Dependencia> construirGrafo(const vector<Instruction> &instrucoes, const BlocoBasico &bloco) {
    vector<Dependencia> grafo;

    for (size_t i = bloco.inicio; i <= bloco.fim; i++) {
        int escritoI = registradorEscrito(instrucoes[i]);

        for (size_t j = i + 1; j <= bloco.fim; j++) {
            int escritoJ = registradorEscrito(instrucoes[j]);

            // RAW: j le o que i escreve
            if (escritoI != -1) {
                for (int reg : registradoresLidos(instrucoes[j])) {
                    if (reg == escritoI) { grafo.push_back({i, j, "RAW"}); break; }
                }
            }

            // WAR: j escreve um registrador que i leu -- i tem que
            // continuar antes, senao i passaria a ler o valor novo de j
            if (escritoJ != -1) {
                for (int reg : registradoresLidos(instrucoes[i])) {
                    if (reg == escritoJ) { grafo.push_back({i, j, "WAR"}); break; }
                }
            }

            // WAW: as duas escrevem o mesmo registrador -- quem escreve
            // por ultimo tem que continuar sendo quem escreve por ultimo
            if (escritoI != -1 && escritoI == escritoJ) {
                grafo.push_back({i, j, "WAW"});
            }

            // MEM: acesso a memoria que pode se sobrepor, com pelo menos
            // uma escrita envolvida (load-load nao precisa manter ordem)
            if (acessaMemoria(instrucoes[i]) && acessaMemoria(instrucoes[j]) &&
                (escreveNaMemoria(instrucoes[i]) || escreveNaMemoria(instrucoes[j])) &&
                podemSobrepor(instrucoes, i, j)) {
                grafo.push_back({i, j, "MEM"});
            }
        }
    }

    return grafo;
}

static vector<Dependencia> filtrarGrafo(const vector<Dependencia> &grafo, const vector<size_t> &indices) {
    vector<Dependencia> filtrado;
    for (const Dependencia &dep : grafo) {
        bool deNoConjunto = find(indices.begin(), indices.end(), dep.de) != indices.end();
        bool paraNoConjunto = find(indices.begin(), indices.end(), dep.para) != indices.end();
        if (deNoConjunto && paraNoConjunto) filtrado.push_back(dep);
    }
    return filtrado;
}

// Util tanto pra "quem pode ser movida pro slot" (sink do bloco inteiro)
// quanto, em outros contextos, pra achar quem pode ser a ultima de uma
// ordem: "candidato" nao tem ninguem em "conjunto" que precise vir depois dele.
static bool ehSink(size_t candidato, const vector<size_t> &conjunto, const vector<Dependencia> &grafo) {
    for (const Dependencia &dep : grafo) {
        if (dep.de != candidato) continue;
        if (find(conjunto.begin(), conjunto.end(), dep.para) != conjunto.end()) return false;
    }
    return true;
}

// Acima disso a busca exaustiva (fatorial no pior caso) deixa de valer a
// pena; mantem a ordem original em vez de travar num bloco gigante.
static const size_t LIMITE_BUSCA_EXAUSTIVA = 8;

static void gerarOrdens(vector<size_t> restantes, vector<size_t> construida,
                         const vector<Dependencia> &grafo, vector<vector<size_t>> &saida) {
    if (restantes.empty()) {
        saida.push_back(construida);
        return;
    }

    for (size_t k = 0; k < restantes.size(); k++) {
        size_t candidato = restantes[k];

        bool pronto = true;
        for (const Dependencia &dep : grafo) {
            if (dep.para != candidato) continue;
            if (find(construida.begin(), construida.end(), dep.de) == construida.end()) {
                pronto = false;
                break;
            }
        }
        if (!pronto) continue;

        vector<size_t> novoRestantes = restantes;
        novoRestantes.erase(novoRestantes.begin() + k);
        construida.push_back(candidato);

        gerarOrdens(novoRestantes, construida, grafo, saida);

        construida.pop_back();
    }
}

vector<vector<size_t>> ordensValidas(const vector<size_t> &indices, const vector<Dependencia> &grafo) {
    vector<vector<size_t>> resultado;
    if (indices.empty()) {
        resultado.push_back(indices);
        return resultado;
    }
    if (indices.size() > LIMITE_BUSCA_EXAUSTIVA) {
        resultado.push_back(indices);
        return resultado;
    }

    vector<Dependencia> filtrado = filtrarGrafo(grafo, indices);
    gerarOrdens(indices, {}, filtrado, resultado);
    return resultado;
}


// Insere na saida, nessa ordem, as instrucoes de "ordem" (indices no vetor
// original), colocando antes de cada uma os nops que ela precisar. Devolve
// quantos nops foram inseridos, pra comparar ordens candidatas pelo custo.
static int inserirOrdem(vector<LinhaCorrigida> &saida, const vector<Instruction> &instrucoes,
                         const vector<size_t> &ordem) {
    int totalNops = 0;
    for (size_t idx : ordem) {
        const Instruction &inst = instrucoes[idx];
        int nops = nopsNecessarios(saida, inst);
        for (int k = 0; k < nops; k++) saida.push_back(criarNop("conflito de dados"));
        totalNops += nops;

        LinhaCorrigida linha;
        linha.inst = inst;
        linha.indiceOriginal = (int)idx;
        linha.marcacao = "";
        saida.push_back(linha);
    }
    return totalNops;
}

// Mesma coisa, mas numa copia da saida -- pra avaliar o custo de uma
// ordem candidata sem comprometer o programa de verdade.
static int custoDaOrdem(const vector<LinhaCorrigida> &saidaAtual, const vector<Instruction> &instrucoes,
                         const vector<size_t> &ordem) {
    vector<LinhaCorrigida> copia = saidaAtual;
    return inserirOrdem(copia, instrucoes, ordem);
}

// Entre as ordens validas de "indices", devolve a de menor custo.
static vector<size_t> melhorOrdem(const vector<LinhaCorrigida> &saida, const vector<Instruction> &instrucoes,
                                   const vector<size_t> &indices, const vector<Dependencia> &grafo) {
    vector<vector<size_t>> ordens = ordensValidas(indices, grafo);
    vector<size_t> melhor = ordens[0];
    int melhorCusto = custoDaOrdem(saida, instrucoes, melhor);
    for (const vector<size_t> &ordem : ordens) {
        int custo = custoDaOrdem(saida, instrucoes, ordem);
        if (custo < melhorCusto) {
            melhorCusto = custo;
            melhor = ordem;
        }
    }
    return melhor;
}

// Colocar "candidato" no slot cria um conflito novo na entrada de um dos
// dois caminhos do desvio? Compara com a 1a instrucao de cada caminho,
// que fica a d=1 do slot (igual ao que control.cpp ja faz, so que aqui
// o "slot" nao e mais instrucoes[desvio.indice+1], e a propria candidata).
static bool criaConflitoNoSlot(const Instruction &candidato, const vector<Instruction> &instrucoes,
                                const Desvio &desvio) {
    int escrito = registradorEscrito(candidato);
    if (escrito == -1) return false;

    if (desvio.condicional) {
        size_t depoisDoSlot = desvio.indice + 2;
        if (depoisDoSlot < instrucoes.size()) {
            for (int reg : registradoresLidos(instrucoes[depoisDoSlot])) {
                if (reg == escrito) return true;
            }
        }
    }

    if (desvio.indiceAlvo != -1) {
        for (int reg : registradoresLidos(instrucoes[desvio.indiceAlvo])) {
            if (reg == escrito) return true;
        }
    }

    return false;
}

static void reordenarBloco(const vector<Instruction> &instrucoes, const BlocoBasico &bloco,
                            const vector<Desvio> &desvios, vector<LinhaCorrigida> &saida) {
    vector<size_t> todos;
    for (size_t i = bloco.inicio; i <= bloco.fim; i++) todos.push_back(i);

    vector<Dependencia> grafo = construirGrafo(instrucoes, bloco);

    bool temDesvio = ehDesvio(instrucoes[bloco.fim]);
    if (!temDesvio) {
        // Bloco sem desvio (ex: ultimo bloco do programa): so reordena
        // pra reduzir nops internos, sem slot de atraso pra preencher.
        inserirOrdem(saida, instrucoes, melhorOrdem(saida, instrucoes, todos, grafo));
        return;
    }

    vector<size_t> permutavel(todos.begin(), todos.end() - 1); // tudo, menos o desvio

    const Desvio *desvioInfo = nullptr;
    for (const Desvio &d : desvios) {
        if (d.indice == bloco.fim) { desvioInfo = &d; break; }
    }

    int escritoPeloDesvio = registradorEscrito(instrucoes[bloco.fim]);

    vector<size_t> candidatos;
    for (size_t c : permutavel) {
        if (ehSink(c, todos, grafo)) candidatos.push_back(c);
    }
    sort(candidatos.begin(), candidatos.end(), greater<size_t>());

    int melhorCusto = INT_MAX;
    vector<size_t> melhorOrdemAntes;
    size_t slotEscolhido = SIZE_MAX; // SIZE_MAX = nenhuma candidata segura, usa nop

    for (size_t candidato : candidatos) {
        // Se o desvio escreve um registrador que a candidata le, colocar
        // ela logo depois (d=1, sem como inserir nop ali sem "quebrar" o
        // slot) seria um hazard -- descarta.
        if (escritoPeloDesvio != -1) {
            bool leDoDesvio = false;
            for (int reg : registradoresLidos(instrucoes[candidato])) {
                if (reg == escritoPeloDesvio) { leDoDesvio = true; break; }
            }
            if (leDoDesvio) continue;
        }

        if (desvioInfo != nullptr && criaConflitoNoSlot(instrucoes[candidato], instrucoes, *desvioInfo)) {
            continue;
        }

        vector<size_t> restante;
        for (size_t c : permutavel) if (c != candidato) restante.push_back(c);

        vector<size_t> ordem = melhorOrdem(saida, instrucoes, restante, grafo);
        int custo = custoDaOrdem(saida, instrucoes, ordem);
        if (custo < melhorCusto) {
            melhorCusto = custo;
            melhorOrdemAntes = ordem;
            slotEscolhido = candidato;
        }
    }

    if (slotEscolhido == SIZE_MAX) {
        // Nenhuma candidata segura: reordena so o que vem antes do
        // desvio (sem mover ninguem pro slot) e preenche o slot com nop,
        // exatamente como o R3/R4 ja fazia.
        melhorOrdemAntes = melhorOrdem(saida, instrucoes, permutavel, grafo);
    }

    inserirOrdem(saida, instrucoes, melhorOrdemAntes);

    const Instruction &desvioInst = instrucoes[bloco.fim];
    int nopsDesvio = nopsNecessarios(saida, desvioInst);
    for (int k = 0; k < nopsDesvio; k++) saida.push_back(criarNop("conflito de dados"));

    LinhaCorrigida linhaDesvio;
    linhaDesvio.inst = desvioInst;
    linhaDesvio.indiceOriginal = (int)bloco.fim;
    linhaDesvio.marcacao = "";
    saida.push_back(linhaDesvio);

    if (slotEscolhido != SIZE_MAX) {
        LinhaCorrigida linhaSlot;
        linhaSlot.inst = instrucoes[slotEscolhido];
        linhaSlot.indiceOriginal = (int)slotEscolhido;
        linhaSlot.marcacao = "<- movida para o slot de atraso (originalmente em " +
                             toHex(instrucoes[slotEscolhido].address, 8) + ")";
        saida.push_back(linhaSlot);
    } else {
        saida.push_back(criarNop("slot de atraso"));
    }
}

vector<LinhaCorrigida> reordenarComNops(const vector<Instruction> &instrucoes) {
    vector<Desvio> desvios = detectarDesvios(instrucoes);
    vector<BlocoBasico> blocos = calcularBlocos(instrucoes, desvios);

    vector<LinhaCorrigida> saida;
    for (const BlocoBasico &bloco : blocos) {
        reordenarBloco(instrucoes, bloco, desvios, saida);
    }
    return saida;
}