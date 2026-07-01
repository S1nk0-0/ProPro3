#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// ── Nodo del árbol (genérico) ────────────────────────────────
// Programación Genérica: T es el tipo de valor guardado en cada nodo.
// En este proyecto T = int (id de película), pero la estructura no
// depende de eso.
template <typename T>
struct TrieNode {
    unordered_map<char, TrieNode<T>*> children;
    unordered_set<T> values;   // valores que contienen este substring
};

// ── Suffix Trie genérico ───────────────────────────────────────
// Inserta todos los sufijos de cada palabra para permitir
// busqueda de substrings: "bar" encuentra "barco", "embarcar", etc.
template <typename T>
class SuffixTrie {
private:
    TrieNode<T>* root;

    void insertString(const string& s, const T& value) {
        TrieNode<T>* cur = root;
        for (char c : s) {
            if (!cur->children.count(c))
                cur->children[c] = new TrieNode<T>();
            cur = cur->children[c];
            cur->values.insert(value);  // guarda el valor en CADA nodo del camino
        }
    }

    static void mergeNode(TrieNode<T>* dst, TrieNode<T>* src) {
        for (auto& par : src->children) {
            char c = par.first;
            TrieNode<T>* srcChild = par.second;
            if (!dst->children.count(c)) dst->children[c] = new TrieNode<T>();
            TrieNode<T>* dstChild = dst->children[c];
            dstChild->values.insert(srcChild->values.begin(), srcChild->values.end());
            mergeNode(dstChild, srcChild);
        }
    }

public:
    SuffixTrie() { root = new TrieNode<T>(); }

    // inserta "barco" → llama insertString con "barco","arco","rco","co","o"
    void indexWord(const string& word, const T& value) {
        for (int i = 0; i < (int)word.size(); i++)
            insertString(word.substr(i), value);
    }

    // recorre el arbol letra por letra y devuelve los valores del ultimo nodo
    unordered_set<T> search(const string& query) const {
        TrieNode<T>* cur = root;
        for (char c : query) {
            if (!cur->children.count(c)) return {};
            cur = cur->children[c];
        }
        return cur->values;
    }

    // Programación Paralela: cada hilo indexa su propia porción del
    // dataset en un SuffixTrie separado (sin memoria compartida), y luego
    // se combinan aquí, evitando condiciones de carrera durante la carga.
    void merge(SuffixTrie<T>& otro) {
        mergeNode(root, otro.root);
    }
};
