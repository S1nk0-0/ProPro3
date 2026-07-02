#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <thread>
#include <memory>
#include <functional>
#include <filesystem>
#include "Trie.cpp"
using namespace std;

struct Movie {
    int id = -1;
    int releaseYear = 0;
    string title;
    string origin;
    string cast;
    string director;
    string genre;
    string wikiPage;
    string plot;
};

vector<Movie> movies;
SuffixTrie<int> trie;           // todos los campos
SuffixTrie<int> titleTrie;      // solo titulos (substrings)
SuffixTrie<int> directorTrie;   // solo director (para busqueda por tag)
SuffixTrie<int> castTrie;       // solo cast (para busqueda por tag)
SuffixTrie<int> genreTrie;      // solo genero (para busqueda por tag)
unordered_map<string, unordered_set<int>> wordTitleIndex; // palabra exacta → IDs

/*
=====================================================================
    PRE-PROCESAMIENTO: eliminamos los chars que no nos sirven y
                       separamos las palabras en un vector
=====================================================================
*/
vector<string> preprocesar(const string& texto) {
    vector<string> tokens;
    string actual;

    for (char c : texto) {
        if (isalnum((unsigned char)c)) {
            actual += tolower((unsigned char)c);
        } else if (!actual.empty()) {
            if (actual.size() > 1) tokens.push_back(actual);
            actual.clear();
        }
    }
    if (actual.size() > 1) tokens.push_back(actual);
    return tokens;
}

// ─────────────────────────────────────────────────────────────
//  PARA LEER BIEN EL CSV
//  maneja campos entre comillas con comas y saltos de linea
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

bool esTituloValido(const string& t) {
    if (t.empty() || t.size() > 150) return false;
    // detectar concatenaciones tipo "TheThe": minuscula seguida de mayuscula sin espacio
    for (size_t i = 1; i < t.size(); i++)
        if (islower((unsigned char)t[i-1]) && isupper((unsigned char)t[i]))
            return false;
    // primer caracter no-espacio debe ser mayuscula o digito
    for (char c : t)
        if (!isspace((unsigned char)c))
            return isupper((unsigned char)c) || isdigit((unsigned char)c);
    return false;
}

// ── Patrón de diseño: Factory Method ──────────────────────────
// Encapsula la construcción/validación de un Movie a partir de una fila
// cruda del CSV. Devuelve ok=false si la fila debe descartarse.
Movie crearPelicula(int id, const vector<string>& c,
                     int iTitle, int iPlot, int iGenre, int iDirector,
                     int iCast, int iOrigin, int iWikiPage, int iReleaseYear,
                     bool& ok) {
    auto get = [&](int idx) -> string {
        return (idx >= 0 && idx < (int)c.size()) ? c[idx] : "";
    };

    Movie m;
    ok = esTituloValido(get(iTitle));
    if (!ok) return m;

    m.id        = id;
    m.title     = get(iTitle);
    m.plot      = get(iPlot);
    m.genre     = get(iGenre);
    m.director  = get(iDirector);
    m.cast      = get(iCast);
    m.origin    = get(iOrigin);
    m.wikiPage  = get(iWikiPage);
    try { if (iReleaseYear >= 0) m.releaseYear = stoi(get(iReleaseYear)); } catch (...) {}
    return m;
}

// ─────────────────────────────────────────────────────────────
//  INDEXACION (usada tanto en secuencial como en paralelo)
// ─────────────────────────────────────────────────────────────
void indexarRango(size_t desde, size_t hasta,
                   SuffixTrie<int>& trieOut, SuffixTrie<int>& titleOut,
                   SuffixTrie<int>& directorOut, SuffixTrie<int>& castOut,
                   SuffixTrie<int>& genreOut,
                   unordered_map<string, unordered_set<int>>& wordTitleOut) {
    for (size_t i = desde; i < hasta; i++) {
        const Movie& m = movies[i];
        int id = m.id;

        for (const string& tok : preprocesar(m.title)) {
            trieOut.indexWord(tok, id);
            titleOut.indexWord(tok, id);
            wordTitleOut[tok].insert(id);  // palabra exacta del titulo
        }
        // solo indexamos palabras cortas del plot para no saturar el trie
        for (const string& tok : preprocesar(m.plot))
            if (tok.size() <= 12) trieOut.indexWord(tok, id);
        for (const string& tok : preprocesar(m.genre)) {
            trieOut.indexWord(tok, id);
            genreOut.indexWord(tok, id);
        }
        for (const string& tok : preprocesar(m.director)) {
            trieOut.indexWord(tok, id);
            directorOut.indexWord(tok, id);
        }
        for (const string& tok : preprocesar(m.cast)) {
            trieOut.indexWord(tok, id);
            castOut.indexWord(tok, id);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  CARGA DEL CSV + CONSTRUCCION DEL INDICE (en paralelo)
// ─────────────────────────────────────────────────────────────
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
    while (leerFilaCSV(file, linea)) {
        if (linea.empty()) continue;
        vector<string> c = revisarCSV(linea);

        bool ok;
        Movie m = crearPelicula(id, c, iTitle, iPlot, iGenre, iDirector, iCast,
                                 iOrigin, iWikiPage, iReleaseYear, ok);
        if (!ok) continue;

        movies.push_back(m);
        id++;
        if (id % 1000 == 0) cout << id << " peliculas cargadas...\r" << flush;
    }
    cout << movies.size() << " peliculas cargadas.        \n";

    // ── Programación Paralela ──────────────────────────────────
    // Se reparte el vector de peliculas en N bloques (N = nucleos
    // disponibles). Cada hilo construye sus propios tries locales
    // (sin memoria compartida) y al final se fusionan en los tries
    // globales, evitando condiciones de carrera.
    unsigned nHilos = max(1u, thread::hardware_concurrency());
    nHilos = (unsigned)min<size_t>(nHilos, max<size_t>(1, movies.size()));
    size_t total = movies.size();
    size_t porHilo = (total + nHilos - 1) / nHilos;

    vector<SuffixTrie<int>> trieParcial(nHilos), titleParcial(nHilos);
    vector<SuffixTrie<int>> directorParcial(nHilos), castParcial(nHilos), genreParcial(nHilos);
    vector<unordered_map<string, unordered_set<int>>> wordTitleParcial(nHilos);

    vector<thread> hilos;
    for (unsigned t = 0; t < nHilos; t++) {
        size_t desde = t * porHilo;
        size_t hasta = min(total, desde + porHilo);
        if (desde >= hasta) continue;
        hilos.emplace_back(indexarRango, desde, hasta,
                            ref(trieParcial[t]), ref(titleParcial[t]),
                            ref(directorParcial[t]), ref(castParcial[t]), ref(genreParcial[t]),
                            ref(wordTitleParcial[t]));
    }
    for (auto& h : hilos) h.join();

    for (unsigned t = 0; t < nHilos; t++) {
        trie.merge(trieParcial[t]);
        titleTrie.merge(titleParcial[t]);
        directorTrie.merge(directorParcial[t]);
        castTrie.merge(castParcial[t]);
        genreTrie.merge(genreParcial[t]);
        for (auto& par : wordTitleParcial[t])
            wordTitleIndex[par.first].insert(par.second.begin(), par.second.end());
    }

    return true;
}

// palabras tan comunes que matchean en casi todo → ignorar en el query
const unordered_set<string> STOP_WORDS = {
    "the","a","an","at","in","of","to","is","it","he","she","his","her",
    "and","or","on","by","be","as","we","do","if","so","up","no","me",
    "my","am","us","go","was","are","for","not","but","all","its","our",
    "from","with","that","this","they","them","their","there","then",
    "than","had","has","have","been","who","what","when","where","how"
};

// ── Patrón de diseño: Strategy ─────────────────────────────────
// Encapsula el algoritmo de ranking, para poder cambiarlo sin tocar
// el codigo que arma los conteos de coincidencias (buscar()).
struct IEstrategiaRanking {
    virtual ~IEstrategiaRanking() = default;
    virtual vector<int> rankear(const unordered_map<int,int>& exactTitle,
                                 const unordered_map<int,int>& substrTitle,
                                 const unordered_map<int,int>& anyField,
                                 int nTokens, const string& rawQuery) const = 0;
};

struct RankingPorCobertura : IEstrategiaRanking {
    vector<int> rankear(const unordered_map<int,int>& exactTitle,
                         const unordered_map<int,int>& substrTitle,
                         const unordered_map<int,int>& anyField,
                         int nTokens, const string& rawQuery) const override {
        unordered_map<int, int> score;
        for (auto& par : exactTitle)  score[par.first] += par.second * 1000 / nTokens;
        for (auto& par : substrTitle) score[par.first] += par.second *  100 / nTokens;
        for (auto& par : anyField)    score[par.first] += par.second *   10 / nTokens;
        for (auto& par : score)
            score[par.first] -= (int)preprocesar(movies[par.first].title).size();

        string queryLower = rawQuery;
        transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
        queryLower.erase(0, queryLower.find_first_not_of(" \t"));
        queryLower.erase(queryLower.find_last_not_of(" \t") + 1);
        for (auto& par : score) {
            string titleLower = movies[par.first].title;
            transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
            if (titleLower == queryLower) score[par.first] += 10000;
        }

        vector<pair<int,int>> ordenado;
        for (auto& par : score) ordenado.push_back({par.second, par.first});
        sort(ordenado.rbegin(), ordenado.rend());

        vector<int> resultado;
        for (auto& par : ordenado) resultado.push_back(par.second);
        return resultado;
    }
};

unique_ptr<IEstrategiaRanking> estrategiaRanking = make_unique<RankingPorCobertura>();

// ─────────────────────────────────────────────────────────────
//  BUSQUEDA (por texto libre, en titulo/sinopsis/genero/director/cast)
// ─────────────────────────────────────────────────────────────
vector<int> buscar(const string& query) {
    vector<string> tokens = preprocesar(query);
    if (tokens.empty()) return {};

    // filtrar stop words, pero solo si quedan tokens utiles
    vector<string> filtrados;
    for (const string& t : tokens)
        if (!STOP_WORDS.count(t)) filtrados.push_back(t);
    if (!filtrados.empty()) tokens = filtrados;

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

    return estrategiaRanking->rankear(exactTitle, substrTitle, anyField, (int)tokens.size(), query);
}

// ─────────────────────────────────────────────────────────────
//  BUSQUEDA POR TAG (director / genero / cast)
// ─────────────────────────────────────────────────────────────
enum class Tag { DIRECTOR, GENERO, CAST };

vector<int> buscarPorTag(Tag campo, const string& valor) {
    vector<string> tokens = preprocesar(valor);
    if (tokens.empty()) return {};

    SuffixTrie<int>* indice = &directorTrie;
    if (campo == Tag::GENERO) indice = &genreTrie;
    if (campo == Tag::CAST)   indice = &castTrie;

    unordered_map<int,int> conteo;
    for (const string& tok : tokens)
        for (int id : indice->search(tok)) conteo[id]++;

    vector<pair<int,int>> ordenado;
    for (auto& par : conteo) ordenado.push_back({par.second, par.first});
    sort(ordenado.rbegin(), ordenado.rend());

    vector<int> resultado;
    for (auto& par : ordenado) resultado.push_back(par.second);
    return resultado;
}

// ─────────────────────────────────────────────────────────────
//  PELICULAS SIMILARES (algoritmo propio: similitud de Jaccard
//  sobre genero + director + cast tokenizados)
// ─────────────────────────────────────────────────────────────
unordered_set<string> tagTokens(const Movie& m) {
    unordered_set<string> tokens;
    for (auto& t : preprocesar(m.genre)) tokens.insert(t);
    for (auto& t : preprocesar(m.director)) tokens.insert(t);
    for (auto& t : preprocesar(m.cast)) tokens.insert(t);
    return tokens;
}

vector<int> similares(int movieId, int topN = 10) {
    unordered_set<string> base = tagTokens(movies[movieId]);
    if (base.empty()) return {};

    vector<pair<double,int>> scored;
    for (const Movie& m : movies) {
        if (m.id == movieId) continue;
        unordered_set<string> other = tagTokens(m);
        if (other.empty()) continue;

        int interseccion = 0;
        for (const string& t : base) if (other.count(t)) interseccion++;
        if (interseccion == 0) continue;
        int unionSize = (int)(base.size() + other.size()) - interseccion;

        scored.push_back({(double)interseccion / unionSize, m.id});
    }

    sort(scored.rbegin(), scored.rend());
    vector<int> resultado;
    for (int i = 0; i < (int)scored.size() && i < topN; i++)
        resultado.push_back(scored[i].second);
    return resultado;
}

// ── Patrón de diseño: Singleton + Observer ─────────────────────
// Sesion guarda el estado del usuario (likes / ver mas tarde) y persiste
// en disco. Es unica durante la ejecucion (Singleton). Cuando se agrega
// un Like, notifica a los observadores registrados (Observer) para que,
// por ejemplo, se recalculen las recomendaciones.
class Sesion {
public:
    static Sesion& instancia() {
        static Sesion s;
        return s;
    }

    const vector<int>& likes() const { return likesIds; }
    const vector<int>& verMasTarde() const { return verMasTardeIds; }

    bool tieneLike(int id) const { return likesSet.count(id) > 0; }
    bool enVerMasTarde(int id) const { return verMasTardeSet.count(id) > 0; }

    void darLike(int id) {
        if (likesSet.count(id)) return;
        likesSet.insert(id);
        likesIds.push_back(id);
        guardar(likesIds, "likes.txt");
        for (auto& obs : observadoresLike) obs(id);
    }

    void agregarVerMasTarde(int id) {
        if (verMasTardeSet.count(id)) return;
        verMasTardeSet.insert(id);
        verMasTardeIds.push_back(id);
        guardar(verMasTardeIds, "watch_later.txt");
    }

    void suscribirLike(function<void(int)> obs) { observadoresLike.push_back(std::move(obs)); }

private:
    Sesion() {
        cargar(likesIds, likesSet, "likes.txt");
        cargar(verMasTardeIds, verMasTardeSet, "watch_later.txt");
    }

    vector<int> likesIds, verMasTardeIds;
    unordered_set<int> likesSet, verMasTardeSet;
    vector<function<void(int)>> observadoresLike;

    // descarta ids invalidos (archivo editado a mano o dataset distinto)
    static void cargar(vector<int>& ids, unordered_set<int>& idsSet, const string& archivo) {
        ifstream f(archivo);
        int id;
        while (f >> id)
            if (id >= 0 && id < (int)movies.size() && !idsSet.count(id)) {
                idsSet.insert(id);
                ids.push_back(id);
            }
    }
    static void guardar(const vector<int>& ids, const string& archivo) {
        ofstream f(archivo, ios::trunc);
        for (int id : ids) f << id << "\n";
    }
};

// ─────────────────────────────────────────────────────────────
//  RECOMENDACIONES (cache que se recalcula via Observer)
// ─────────────────────────────────────────────────────────────
vector<int> recomendaciones;

void recalcularRecomendaciones(int) {
    unordered_map<int,int> puntaje;
    unordered_set<int> yaVistos(Sesion::instancia().likes().begin(), Sesion::instancia().likes().end());

    for (int likedId : Sesion::instancia().likes()) {
        vector<int> sim = similares(likedId, 15);
        int peso = (int)sim.size();
        for (int id : sim) {
            if (yaVistos.count(id)) continue;
            puntaje[id] += peso--;
        }
    }

    vector<pair<int,int>> ordenado;
    for (auto& par : puntaje) ordenado.push_back({par.second, par.first});
    sort(ordenado.rbegin(), ordenado.rend());

    recomendaciones.clear();
    for (auto& par : ordenado) recomendaciones.push_back(par.second);
}

// ─────────────────────────────────────────────────────────────
//  INTERFAZ
// ─────────────────────────────────────────────────────────────
void mostrarDetalle(int id) {
    const Movie& m = movies[id];
    cout << "\n  Titulo:   " << m.title << "\n";
    cout << "  Genero:   " << (m.genre.empty()    ? "N/A" : m.genre)    << "\n";
    cout << "  Director: " << (m.director.empty() ? "N/A" : m.director) << "\n";
    cout << "  Sinopsis: " << m.plot << "\n";
}

void menuDetalle(int id) {
    while (true) {
        mostrarDetalle(id);
        cout << "\n  [1] Like" << (Sesion::instancia().tieneLike(id) ? " (ya dado)" : "")
             << "\n  [2] Ver mas tarde" << (Sesion::instancia().enVerMasTarde(id) ? " (ya agregada)" : "")
             << "\n  [0] Volver\n> ";
        string op;
        if (!getline(cin, op)) return;
        if (op == "1") { Sesion::instancia().darLike(id); cout << "Like agregado.\n"; }
        else if (op == "2") { Sesion::instancia().agregarVerMasTarde(id); cout << "Agregada a Ver mas tarde.\n"; }
        else if (op == "0") return;
        else cout << "Opcion invalida.\n";
    }
}

// Muestra resultados de 5 en 5, con opcion de ver 5 mas o abrir el detalle
void mostrarResultadosPaginados(const vector<int>& ids) {
    if (ids.empty()) { cout << "Sin resultados.\n"; return; }

    size_t offset = 0;
    const size_t pagina = 5;
    while (true) {
        size_t hasta = min(ids.size(), offset + pagina);
        for (size_t i = offset; i < hasta; i++) {
            const Movie& m = movies[ids[i]];
            cout << "[" << i + 1 << "] " << m.title << " (" << m.releaseYear << ")\n";
        }

        cout << "\n  Numero = ver detalle";
        if (hasta < ids.size()) cout << " | [m] ver 5 mas";
        cout << " | [0] volver\n> ";

        string op;
        if (!getline(cin, op)) return;
        if (op == "0") return;
        if (op == "m" && hasta < ids.size()) { offset = hasta; continue; }

        try {
            int sel = stoi(op);
            if (sel > (int)offset && sel <= (int)hasta) { menuDetalle(ids[sel - 1]); continue; }
        } catch (...) {}
        cout << "Opcion invalida.\n";
    }
}

void mostrarBienvenida() {
    if (!Sesion::instancia().verMasTarde().empty()) {
        cout << "\n-- Ver mas tarde --\n";
        for (int id : Sesion::instancia().verMasTarde()) cout << "  * " << movies[id].title << "\n";
    }
    if (!recomendaciones.empty()) {
        cout << "\n-- Porque te gusto algo similar --\n";
        int i = 0;
        for (int id : recomendaciones) { cout << "  * " << movies[id].title << "\n"; if (++i >= 5) break; }
    }
}

void menuPrincipal() {
    while (true) {
        cout << "\n=== Menu ===\n"
             << "  [1] Buscar peliculas\n"
             << "  [2] Buscar por tag (director/genero/cast)\n"
             << "  [3] Ver mi lista (Ver mas tarde)\n"
             << "  [4] Ver recomendaciones\n"
             << "  [0] Salir\n> ";
        string op;
        if (!getline(cin, op)) return;

        if (op == "1") {
            cout << "Buscar (titulo, sinopsis, frase o sub-palabra): ";
            string q;
            if (!getline(cin, q)) return;
            if (preprocesar(q).empty()) { cout << "Ingresa una busqueda valida.\n"; continue; }
            mostrarResultadosPaginados(buscar(q));
        } else if (op == "2") {
            cout << "Buscar por: [1] Director  [2] Genero  [3] Cast\n> ";
            string t;
            if (!getline(cin, t)) return;
            Tag campo;
            if (t == "1") campo = Tag::DIRECTOR;
            else if (t == "2") campo = Tag::GENERO;
            else if (t == "3") campo = Tag::CAST;
            else { cout << "Opcion invalida.\n"; continue; }

            cout << "Valor a buscar: ";
            string valor;
            if (!getline(cin, valor)) return;
            if (preprocesar(valor).empty()) { cout << "Ingresa un valor valido.\n"; continue; }
            mostrarResultadosPaginados(buscarPorTag(campo, valor));
        } else if (op == "3") {
            cout << "\n=== Ver mas tarde ===\n";
            mostrarResultadosPaginados(Sesion::instancia().verMasTarde());
        } else if (op == "4") {
            cout << "\n=== Recomendaciones ===\n";
            if (Sesion::instancia().likes().empty()) cout << "Dale Like a alguna pelicula primero.\n";
            else mostrarResultadosPaginados(recomendaciones);
        } else if (op == "0") {
            return;
        } else {
            cout << "Opcion invalida.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::filesystem::path exeDir = std::filesystem::path(argv[0]).parent_path();
    string archivo = (argc > 1) ? argv[1] : (exeDir / ".." / "wiki_movie_plots_deduped.csv").string();

    cout << "=== Streaming Platform ===\n";
    if (!cargarCSV(archivo)) return 1;

    Sesion::instancia().suscribirLike(recalcularRecomendaciones);
    recalcularRecomendaciones(0);

    mostrarBienvenida();
    menuPrincipal();
    return 0;
}
