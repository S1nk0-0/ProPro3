#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

template <typename T>
struct TrieNode {
    unordered_map<char, TrieNode<T>*> children;
    unordered_set<T> values;   // valores que contienen este substring

    ~TrieNode() {
        for (auto& par : children) delete par.second;
    }
};


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
            auto it = dst->children.find(par.first);
            if (it == dst->children.end()) {
                dst->children[par.first] = par.second;
                par.second = nullptr;   // dst es el nuevo dueño del subarbol
            } else {
                it->second->values.insert(par.second->values.begin(), par.second->values.end());
                mergeNode(it->second, par.second);
            }
        }
    }

public:
    SuffixTrie() { root = new TrieNode<T>(); }
    ~SuffixTrie() { delete root; }

    SuffixTrie(const SuffixTrie&) = delete;
    SuffixTrie& operator=(const SuffixTrie&) = delete;

    void indexWord(const string& word, const T& value) {
        for (int i = 0; i < (int)word.size(); i++)
            insertString(word.substr(i), value);
    }

    unordered_set<T> search(const string& query) const {
        TrieNode<T>* cur = root;
        for (char c : query) {
            if (!cur->children.count(c)) return {};
            cur = cur->children[c];
        }
        return cur->values;
    }

    void merge(SuffixTrie<T>& otro) {
        mergeNode(root, otro.root);
    }
};
