#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include "Trie.cpp"
using namespace std;

struct Movie {
    int id;
    int releaseYear;
    string title;
    string origin;
    string cast;
    string director;
    string genre;
    string wikiPage;
    string plot;
};

vector<Movie> movies;
SuffixTrie    trie;           // todos los campos
SuffixTrie    titleTrie;      // solo titulos (substrings)
unordered_map<string, unordered_set<int>> wordTitleIndex; // palabra exacta → IDs

/*
PRE-PROCESAMIENTO: eliminamos los chars que no nos
sirven y separamos las palabras en un vector
*/
vector<string> preprocesar(const string& texto) {
    vector<string> tokens;
    string actual;

    for (char c : texto) {
        if (isalnum(c)) {
            actual += tolower(c);
        } else if (!actual.empty()) {
            if (actual.size() > 1) tokens.push_back(actual);
            actual.clear();
        }
    }
    if (actual.size() > 1) tokens.push_back(actual);
    return tokens;
}



// lee una fila completa del CSV aunque el plot tenga saltos de linea
// maneja comillas escapadas ("") para no unir filas por error
bool leerFilaCSV(ifstream& file, string& linea) {
    if (!getline(file, linea)) return false;
    bool inQuotes = false;
    for (size_t i = 0; i < linea.size(); i++) {
        if (linea[i] == '"') {
            if (i + 1 < linea.size() && linea[i+1] == '"') i++; // "" = comilla literal
            else inQuotes = !inQuotes;
        }
    }
    while (inQuotes) {
        string extra;
        if (!getline(file, extra)) break;
        linea += '\n' + extra;
        for (size_t i = 0; i < extra.size(); i++) {
            if (extra[i] == '"') {
                if (i + 1 < extra.size() && extra[i+1] == '"') i++;
                else inQuotes = !inQuotes;
            }
        }
    }
    return true;
}

//  PARA LEER BIEN EL CSV
//  maneja campos entre comillas con comas y saltos de linea
vector<string> revisarCSV(const string& linea) {
    vector<string> campos;
    string campo;
    bool enComillas = false;

    for (size_t i = 0; i < linea.size(); i++) {
        char c = linea[i];
        if (c == '"') {
            enComillas = !enComillas;
        } else if (c == ',' && !enComillas) {
            campos.push_back(campo);
            campo.clear();
        } else if (c != '\r') {
            campo += c;
        }
    }
    campos.push_back(campo);
    return campos;
}

bool esTituloValido(const string& t) {
    if (t.empty() || t.size() > 150) return false;
    // detectar concatenaciones tipo "TheThe": minuscula seguida de mayuscula sin espacio
    for (size_t i = 1; i < t.size(); i++)
        if (islower((unsigned char)t[i-1]) && isupper((unsigned char)t[i]))
            return false;
    // primer caracter no-espacio debe ser mayuscula o digito
    for (char c : t)
        if (!isspace(c))
            return isupper(c) || isdigit(c);
    return false;
}

//  CARGA DEL CSV + CONSTRUCCION DEL INDICE
bool cargarCSV(const string& archivo) {
    ifstream file(archivo);
    if (!file.is_open()) {
        cout << "Error: no se pudo abrir \"" << archivo << "\"\n";
        return false;
    }

    string linea;
    getline(file, linea);
    vector<string> headers = revisarCSV(linea);

    int iReleaseYear = -1, iTitle = -1, iOrigin  = -1;
    int iDirector    = -1, iCast  = -1, iGenre   = -1;
    int iWikiPage    = -1, iPlot  = -1;

    for (int i = 0; i < (int)headers.size(); i++) {
        string h = headers[i];
        transform(h.begin(), h.end(), h.begin(), ::tolower);
        h.erase(0, h.find_first_not_of(" \t"));
        h.erase(h.find_last_not_of(" \t") + 1);

        if (h == "release year")      iReleaseYear = i;
        if (h == "title")             iTitle       = i;
        if (h == "origin/ethnicity")  iOrigin      = i;
        if (h == "director")          iDirector    = i;
        if (h == "cast")              iCast        = i;
        if (h == "genre")             iGenre       = i;
        if (h == "wiki page")         iWikiPage    = i;
        if (h == "plot")              iPlot        = i;
    }

    int id = 0;
    while (leerFilaCSV(file, linea)) {  // Bug 3: sin limite
        if (linea.empty()) continue;
        vector<string> c = revisarCSV(linea);

        auto get = [&](int idx) -> string {
            return (idx >= 0 && idx < (int)c.size()) ? c[idx] : "";
        };

        Movie m;
        m.id        = id;
        m.title     = get(iTitle);
        m.plot      = get(iPlot);
        m.genre     = get(iGenre);
        m.director  = get(iDirector);
        m.cast      = get(iCast);
        m.origin    = get(iOrigin);
        try { if (iReleaseYear >= 0) m.releaseYear = stoi(get(iReleaseYear)); } catch (...) {}

        if (!esTituloValido(m.title)) continue;

        for (const string& tok : preprocesar(m.title)) {
            trie.indexWord(tok, id);
            titleTrie.indexWord(tok, id);
            wordTitleIndex[tok].insert(id);  // palabra exacta del titulo
        }
        // Bug 3: solo indexamos palabras cortas del plot para no saturar el trie
        for (const string& tok : preprocesar(m.plot))
            if (tok.size() <= 12) trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.genre))    trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.director)) trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.cast))     trie.indexWord(tok, id);

        movies.push_back(m);
        id++;
        if (id % 1000 == 0) cout << id << " peliculas cargadas...\r" << flush;
    }

    cout << movies.size() << " peliculas cargadas.        \n";
    return true;
}

// palabras tan comunes que matchean en casi todo
const unordered_set<string> STOP_WORDS = {
    "the","a","an","at","in","of","to","is","it","he","she","his","her",
    "and","or","on","by","be","as","we","do","if","so","up","no","me",
    "my","am","us","go","was","are","for","not","but","all","its","our",
    "from","with","that","this","they","them","their","there","then",
    "than","had","has","have","been","who","what","when","where","how"
};

//  BUSQUEDA
//  rankea por cantidad de tokens que coinciden,
//  titulo primero, luego otros campos
vector<int> buscar(const string& query) {
    vector<string> tokens = preprocesar(query);
    if (tokens.empty()) return {};

    // filtrar stop words, pero solo si quedan tokens utiles
    vector<string> filtrados;
    for (const string& t : tokens)
        if (!STOP_WORDS.count(t)) filtrados.push_back(t);
    if (!filtrados.empty()) tokens = filtrados;

    int nTokens = (int)tokens.size();
    unordered_map<int, int> exactTitle;   // tokens que son palabras exactas del titulo
    unordered_map<int, int> substrTitle;  // tokens que aparecen como substring en titulo
    unordered_map<int, int> anyField;     // tokens que aparecen en cualquier campo

    for (const string& tok : tokens) {
        auto it = wordTitleIndex.find(tok);
        if (it != wordTitleIndex.end())
            for (int id : it->second) exactTitle[id]++;
        for (int id : titleTrie.search(tok)) substrTitle[id]++;
        for (int id : trie.search(tok))      anyField[id]++;
    }

    // Score = cobertura del query en titulo (primario) + penalidad por titulo largo
    unordered_map<int, int> score;
    for (auto& [id, cnt] : exactTitle)  score[id] += cnt * 1000 / nTokens;
    for (auto& [id, cnt] : substrTitle) score[id] += cnt *  100 / nTokens;
    for (auto& [id, cnt] : anyField)    score[id] += cnt *   10 / nTokens;
    for (auto& [id,   _] : score)
        score[id] -= (int)preprocesar(movies[id].title).size();

    // Bonus por match exacto de string: "The Great Train Robbery" != "The Great K & A Train Robbery"
    string queryLower = query;
    transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    queryLower.erase(0, queryLower.find_first_not_of(" \t"));
    queryLower.erase(queryLower.find_last_not_of(" \t") + 1);
    for (auto& [id, _] : score) {
        string titleLower = movies[id].title;
        transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
        if (titleLower == queryLower) score[id] += 10000;
    }

    vector<pair<int,int>> sorted_results;
    for (auto& [id, s] : score) sorted_results.push_back({s, id});
    sort(sorted_results.rbegin(), sorted_results.rend());

    vector<int> resultado;
    for (auto& [s, id] : sorted_results) resultado.push_back(id);
    return resultado;
}

//  INTERFAZ
void mostrarDetalle(int id) {
    const Movie& m = movies[id];
    cout << "\n  Titulo:   " << m.title << "\n";
    cout << "  Genero:   " << (m.genre.empty()    ? "N/A" : m.genre)    << "\n";
    cout << "  Director: " << (m.director.empty() ? "N/A" : m.director) << "\n";
    cout << "  Sinopsis: " << m.plot << "\n";
}

void mostrarResultados(const vector<int>& ids) {
    if (ids.empty()) { cout << "Sin resultados.\n"; return; }
    for (int i = 0; i < min((int)ids.size(), 10); i++) {
        const Movie& m = movies[ids[i]];
        cout << "[" << i + 1 << "] " << m.title
             << " (" << m.releaseYear << ")\n";
    }
}

int main(int argc, char* argv[]) {
    string exePath = string(argv[0]);
    size_t pos = exePath.find_last_of("/\\");
    string exeDir = (pos != string::npos) ? exePath.substr(0, pos) : ".";
    string archivo = (argc > 1) ? argv[1] : exeDir + "/../wiki_movie_plots_deduped.csv";

    cout << "=== Streaming Platform ===\n";
    if (!cargarCSV(archivo)) return 1;
    string query;
    while (true) {
        cout << "\nBuscar (0 para salir): ";
        getline(cin, query);
        if (query == "0") break;
        // Bug 2: ignorar queries vacias o solo con espacios
        if (preprocesar(query).empty()) { cout << "Ingresa una busqueda valida.\n"; continue; }
        mostrarResultados(buscar(query));
    }
    return 0;
}