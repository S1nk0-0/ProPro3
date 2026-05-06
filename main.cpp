#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include "Trie.cpp"
using namespace std;

struct Movie {

    int id; // del vector
    int releaseYear;
    string title;
    string origin;
    string cast;
    string director;
    string genre;
    string wikiPage;
    string plot;
};

vector<Movie> movies;   // todas las peliculas que tenemos
SuffixTrie    trie;
/*
=====================================================================
    PRE-PROCESAMIENTO: eliminamos los chars que no nos sirven y
                       separamos las palabras en un vector
=====================================================================
*/
vector<string> preprocesar(const string& texto) {
    vector<string> tokens; // vector de palabras para procesar
    string actual; // palabra post preprocesamiento

    for (char c : texto) {
        if (isalnum(c)) { // verifica si el char no es un guion, espacio, etc
            actual += tolower(c);       // lo agregamos y convertimos a minuscula
        } else if (!actual.empty()) { // verificamos que actual no este vacio
            tokens.push_back(actual); // agregamos la palabra a tokens
            actual.clear(); // reiniciamos la palabra
        }
    }
    if (actual.size() > 1) tokens.push_back(actual); // ultima palabra
    return tokens;
}

// ─────────────────────────────────────────────────────────────
//  PARA LEER BIEN EL CSV
//  maneja campos entre comillas que pueden contener comas
//  "titulo normal", "titulo, con coma"  →  2 campos correctos
// ─────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────
//  CARGA DEL CSV + CONSTRUCCION DEL INDICE
// ─────────────────────────────────────────────────────────────
bool cargarCSV(const string& archivo) {
    ifstream file(archivo);


    string linea;
    getline(file, linea);
    vector<string> headers = revisarCSV(linea);

    int iReleaseYear = -1;
    int iTitle       = -1;
    int iOrigin      = -1;
    int iDirector    = -1;
    int iCast        = -1;
    int iGenre       = -1;
    int iWikiPage    = -1;
    int iPlot        = -1;

    for (int i = 0; i < headers.size(); i++) {
        // verificamos que todas las columnas esten bien nombradas
        string h = headers[i];
        transform(h.begin(), h.end(), h.begin(), ::tolower);

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
    while (getline(file, linea)) {
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

        if (m.title.empty()) continue;

        // indexar en el trie
        for (const string& tok : preprocesar(m.title))    trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.plot))     trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.genre))    trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.director)) trie.indexWord(tok, id);
        for (const string& tok : preprocesar(m.cast))     trie.indexWord(tok, id);

        movies.push_back(m);
        id++;
    }

    cout << movies.size() << " peliculas cargadas.\n";
    return true;
}

// ─────────────────────────────────────────────────────────────
//  BUSQUEDA
//  tokeniza el query y hace union de resultados del trie
// ─────────────────────────────────────────────────────────────
vector<int> buscar(const string& query) {
    vector<string> tokens = preprocesar(query);
    unordered_set<int> resultado;

    for (const string& tok : tokens) {
        auto ids = trie.search(tok);
        resultado.insert(ids.begin(), ids.end());
    }

    return vector<int>(resultado.begin(), resultado.end());
}

// ─────────────────────────────────────────────────────────────
//  INTERFAZ
// ─────────────────────────────────────────────────────────────
void mostrarDetalle(int id) {
    const Movie& m = movies[id];
    cout << "\n  Titulo:   " << m.title << "\n";
    cout << "  Genero:   " << (m.genre.empty()   ? "N/A" : m.genre)   << "\n";
    cout << "  Director: " << (m.director.empty() ? "N/A" : m.director) << "\n";
    cout << "  Sinopsis: "<< m.plot << "\n";
}

void mostrarResultados(const vector<int>& ids) ;


int main(int argc, char* argv[]) {
    string archivo = (argc > 1) ? argv[1] : "movies.csv";

    cout << "=== Streaming Platform ===\n";
    if (!cargarCSV(archivo)) return 1;

    string query;
    while (true) {
        cout << "\n[1] Buscar  [0] Salir\n> ";
        int op; cin >> op; cin.ignore();
        if (op == 0) break;

        cout << "Buscar: ";
        getline(cin, query);
        mostrarResultados(buscar(query));
    }
    return 0;
}
