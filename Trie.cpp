#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;
struct TrieNode {
    unordered_map<char, TrieNode*> children;
    unordered_set<int> movieIds;   // peliculas que contienen este substring
};

// Inserta todos los sufijos de cada palabra para permitir
// busqueda de substrings: "bar" encuentra "barco", "embarcar", etc.
class SuffixTrie {
private:
    TrieNode* root;

    void insertString(const string& s, int id) {
        TrieNode* cur = root;
        for (char c : s) {
            if (!cur->children.count(c))
                cur->children[c] = new TrieNode();
            cur = cur->children[c];
            cur->movieIds.insert(id);  // guarda el id en CADA nodo del camino
        }
    }

public:
    SuffixTrie() { root = new TrieNode(); }

    // inserta "barco" → llama insertString con "barco","arco","rco","co","o"
    void indexWord(const string& word, int id) {
        for (int i = 0; i < (int)word.size(); i++)
            insertString(word.substr(i), id);
    }

    // recorre el arbol letra por letra y devuelve los ids del ultimo nodo
    unordered_set<int> search(const string& query) {
        TrieNode* cur = root;
        for (char c : query) {
            if (!cur->children.count(c)) return {};
            cur = cur->children[c];
        }
        return cur->movieIds;
    }
};
