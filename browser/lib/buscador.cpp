#include "buscador.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;


static int extractInt(const string& src, const string& key) {
    size_t p = src.find(key);
    if (p == string::npos) return 0;
    try { return stoi(src.substr(p + key.size())); }
    catch (...) { return 0; }
}

static string captureOut(function<void()> fn) {
    ostringstream oss;
    streambuf* old = cout.rdbuf(oss.rdbuf());
    fn();
    cout.rdbuf(old);
    return oss.str();
}


Buscador::Buscador(const string& directorioIndexacion, const int& f)
    : IndexadorHash(directorioIndexacion), docsOrdenados(),
      formSimilitud(f), c(2.0), k1(1.2), b(0.75) {}

Buscador::Buscador(const Buscador& busc)
    : IndexadorHash(busc), docsOrdenados(busc.docsOrdenados),
      formSimilitud(busc.formSimilitud), c(busc.c), k1(busc.k1), b(busc.b) {}

Buscador::~Buscador() {}

Buscador& Buscador::operator=(const Buscador& busc) {
    if (this == &busc) return *this;
    IndexadorHash::operator=(busc);
    docsOrdenados = busc.docsOrdenados;
    formSimilitud = busc.formSimilitud;
    c = busc.c; k1 = busc.k1; b = busc.b;
    return *this;
}

Buscador::Buscador()
    : IndexadorHash(""), docsOrdenados(), formSimilitud(0), c(2.0), k1(1.2), b(0.75) {}

// ─── Getters / setters ───────────────────────────────────────────────────────

int    Buscador::DevolverFormulaSimilitud() const { return formSimilitud; }
bool   Buscador::CambiarFormulaSimilitud(const int& f) {
    if (f != 0 && f != 1) return false;
    formSimilitud = f; return true;
}
void   Buscador::CambiarParametrosDFR(const double& kc) { c = kc; }
double Buscador::DevolverParametrosDFR() const { return c; }
void   Buscador::CambiarParametrosBM25(const double& kk1, const double& kb) { k1=kk1; b=kb; }
void   Buscador::DevolverParametrosBM25(double& kk1, double& kb) const { kk1=k1; kb=b; }

// ─── buildDocMaps ────────────────────────────────────────────────────────────

void Buscador::buildDocMaps(unordered_map<int,int>& idToLen,
                             unordered_map<int,string>& idToName) const {
    idToLen.clear(); idToName.clear();
    string dStr = captureOut([&]{ const_cast<Buscador*>(this)->ListarDocs(); });
    istringstream diss(dStr);
    string dline;
    while (getline(diss, dline)) {
        size_t idp = dline.find("idDoc: ");
        if (idp == string::npos) continue;
        int did = stoi(dline.substr(idp + 7));
        size_t npp = dline.find("numPalSinParada: ");
        int npsp = (npp != string::npos) ? stoi(dline.substr(npp + 17)) : 1;
        idToLen[did] = max(1, npsp);
        size_t tab = dline.find('\t');
        string fullName = (tab != string::npos) ? dline.substr(0, tab) : dline;
        size_t slash = fullName.rfind('/');
        if (slash != string::npos) fullName = fullName.substr(slash + 1);
        size_t dot = fullName.rfind('.');
        if (dot != string::npos) fullName = fullName.substr(0, dot);
        idToName[did] = fullName;
    }
}

// ─── getColData ──────────────────────────────────────────────────────────────

Buscador::ColData Buscador::getColData() const {
    string s = captureOut([&]{ const_cast<Buscador*>(this)->ListarInfColeccDocs(); });
    int N   = extractInt(s, "numDocs: ");
    int sum = extractInt(s, "numTotalPalSinParada: ");
    return {N, (N > 0) ? (double)sum / N : 1.0};
}

// ─── getTermsQuery ────────────────────────────────────────────────────────────
// Obtiene los términos de la pregunta iterando sobre los del índice de documentos
// y consultando si cada uno está en la pregunta indexada.
// (ImprimirIndexacionPregunta() no imprime los términos en la implementación dada)
unordered_map<string,int> Buscador::getTermsQuery() const {
    unordered_map<string,int> result;

    // Obtenemos todos los términos del índice de documentos
    string allTerms = captureOut([&]{ const_cast<Buscador*>(this)->ListarTerminos(); });
    istringstream tiss(allTerms);
    string line;
    while (getline(tiss, line)) {
        size_t tab = line.find('\t');
        if (tab == string::npos) continue;
        string term = line.substr(0, tab);

        InformacionTerminoPregunta itp;
        if (const_cast<Buscador*>(this)->DevuelvePregunta(term, itp)) {
            // El término está en la pregunta; extraemos ft
            string itpStr = captureOut([&]{ cout << itp; });
            int ft = extractInt(itpStr, "ft: ");
            result[term] = ft;
        }
    }
    return result;
}

// ─── parseTermInfo ────────────────────────────────────────────────────────────

static unordered_map<int,int> parseTermInfo(const string& tStr, int& ft_global, int& fd) {
    ft_global = extractInt(tStr, "Frecuencia total: ");
    fd        = extractInt(tStr, "fd: ");
    unordered_map<int,int> docs;
    size_t pos = 0;
    const string key = "Id.Doc: ";
    while ((pos = tStr.find(key, pos)) != string::npos) {
        size_t after = pos + key.size();
        int docId = stoi(tStr.substr(after));
        size_t tabPos = tStr.find('\t', after);
        int ftd = 0;
        if (tabPos != string::npos)
            ftd = extractInt(tStr.substr(tabPos + 1), "ft: ");
        docs[docId] = ftd;
        pos = after;
    }
    return docs;
}


bool Buscador::buscarDFR(const int& numDocumentos, const int numPregunta) {
    ColData col = getColData();
    int N = col.N;
    if (N == 0) return true;

    // k = términos no parada de la query
    InformacionPregunta infPreg;
    if (!DevuelvePregunta(infPreg)) return false;
    string ipStr = captureOut([&]{ const_cast<Buscador*>(this)->ImprimirPregunta(); });
    int k = extractInt(ipStr, "numTotalPalSinParada: ");
    if (k == 0) return false;

    unordered_map<int,int> idToLen;
    unordered_map<int,string> idToName;
    buildDocMaps(idToLen, idToName);

    auto queryTerms = getTermsQuery();
    unordered_map<int, double> scores;

    for (auto& qtEntry : queryTerms) {
        const string& term = qtEntry.first;
        int ftq = qtEntry.second;

        InformacionTermino it;
        if (!const_cast<Buscador*>(this)->Devuelve(term, it)) continue;
        string tStr = captureOut([&]{ cout << it; });
        int ft_global, fd;
        auto docs = parseTermInfo(tStr, ft_global, fd);
        if (ft_global == 0 || fd == 0) continue;

        double lambda_t = (double)ft_global / N;
        double log1pL   = log2(1.0 + lambda_t);
        double logRatio = (lambda_t > 0.0) ? log2((1.0 + lambda_t) / lambda_t) : 0.0;
        double w_tq     = (double)ftq / k;

        for (auto& dkv : docs) {
            int docId = dkv.first, ftd = dkv.second;
            int l_d = 1;
            auto lit = idToLen.find(docId);
            if (lit != idToLen.end()) l_d = lit->second;
            double f_star = ftd * log2(1.0 + c * col.avr_ld / l_d);
            double w_td   = log1pL + f_star * logRatio;
            scores[docId] += w_tq * w_td;
        }
    }

    vector<pair<int,double>> sdocs(scores.begin(), scores.end());
    sort(sdocs.begin(), sdocs.end(),
         [](const pair<int,double>& a, const pair<int,double>& b){ return a.second > b.second; });
    int added = 0;
    for (auto& kv : sdocs) {
        if (added >= numDocumentos) break;
        docsOrdenados.push(ResultadoRI(kv.second, kv.first, numPregunta));
        ++added;
    }
    return true;
}


bool Buscador::buscarBM25(const int& numDocumentos, const int numPregunta) {
    ColData col = getColData();
    int N = col.N;
    if (N == 0) return true;

    InformacionPregunta infPreg;
    if (!DevuelvePregunta(infPreg)) return false;
    string ipStr = captureOut([&]{ const_cast<Buscador*>(this)->ImprimirPregunta(); });
    if (extractInt(ipStr, "numTotalPalSinParada: ") == 0) return false;

    unordered_map<int,int> idToLen;
    unordered_map<int,string> idToName;
    buildDocMaps(idToLen, idToName);

    auto queryTerms = getTermsQuery();
    unordered_map<int, double> scores;

    for (auto& qtEntry : queryTerms) {
        const string& term = qtEntry.first;
        InformacionTermino it;
        if (!const_cast<Buscador*>(this)->Devuelve(term, it)) continue;
        string tStr = captureOut([&]{ cout << it; });
        int ft_global, fd;
        auto docs = parseTermInfo(tStr, ft_global, fd);
        if (fd == 0) continue;

        double idf = log2((double)(N - fd + 0.5) / (fd + 0.5));

        for (auto& dkv : docs) {
            int docId = dkv.first, ftd = dkv.second;
            int l_d = 1;
            auto lit = idToLen.find(docId);
            if (lit != idToLen.end()) l_d = lit->second;
            double norm = (double)ftd * (k1 + 1.0)
                          / ((double)ftd + k1 * (1.0 - b + b * l_d / col.avr_ld));
            scores[docId] += idf * norm;
        }
    }

    vector<pair<int,double>> sdocs(scores.begin(), scores.end());
    sort(sdocs.begin(), sdocs.end(),
         [](const pair<int,double>& a, const pair<int,double>& b){ return a.second > b.second; });
    int added = 0;
    for (auto& kv : sdocs) {
        if (added >= numDocumentos) break;
        docsOrdenados.push(ResultadoRI(kv.second, kv.first, numPregunta));
        ++added;
    }
    return true;
}


bool Buscador::Buscar(const int& numDocumentos) {
    docsOrdenados = priority_queue<ResultadoRI>();
    string preg;
    if (!DevuelvePregunta(preg)) return false;
    return (formSimilitud == 0) ? buscarDFR(numDocumentos, 0)
                                : buscarBM25(numDocumentos, 0);
}

bool Buscador::Buscar(const string& dirPreguntas, const int& numDocumentos,
                      const int& numPregInicio, const int& numPregFin) {
    docsOrdenados = priority_queue<ResultadoRI>();
    for (int np = numPregInicio; np <= numPregFin; ++np) {
        string fichPreg = dirPreguntas;
        if (!fichPreg.empty() && fichPreg.back() != '/') fichPreg += '/';
        fichPreg += to_string(np) + ".txt";

        ifstream fin(fichPreg);
        if (!fin.is_open()) {
            cerr << "ERROR: no se pudo abrir el fichero de pregunta: " << fichPreg << endl;
            continue;
        }
        ostringstream oss; oss << fin.rdbuf();
        fin.close();
        if (!IndexarPregunta(oss.str())) continue;

        bool ok = (formSimilitud == 0) ? buscarDFR(numDocumentos, np)
                                       : buscarBM25(numDocumentos, np);
        if (!ok) {
            cerr << "ERROR: búsqueda fallida en pregunta " << np << endl;
            return false;
        }
    }
    return true;
}


void Buscador::imprimirResultados(ostream& out, const int& numDocumentos) const {
    unordered_map<int,int> idToLen;
    unordered_map<int,string> idToName;
    buildDocMaps(idToLen, idToName);

    string pregActual;
    const_cast<Buscador*>(this)->DevuelvePregunta(pregActual);
    string formulaStr = (formSimilitud == 0) ? "DFR" : "BM25";
    out.imbue(locale::classic());

    priority_queue<ResultadoRI> copia = docsOrdenados;
    unordered_map<int, int> pos;

    while (!copia.empty()) {
        const ResultadoRI& res = copia.top();
        int np = res.NumPregunta(), docId = (int)res.IdDoc();
        int& p = pos[np];
        if (p >= numDocumentos) { copia.pop(); continue; }

        string nomDoc;
        auto nit = idToName.find(docId);
        nomDoc = (nit != idToName.end()) ? nit->second : to_string(docId);

        string pregStr = (np == 0) ? pregActual : "ConjuntoDePreguntas";
        out << np << " " << formulaStr << " " << nomDoc << " "
            << p << " " << res.VSimilitud() << " " << pregStr << "\n";
        ++p;
        copia.pop();
    }
}

void Buscador::ImprimirResultadoBusqueda(const int& numDocumentos) const {
    imprimirResultados(cout, numDocumentos);
}

bool Buscador::ImprimirResultadoBusqueda(const int& numDocumentos,
                                          const string& nombreFichero) const {
    ofstream fout(nombreFichero);
    if (!fout.is_open()) {
        cerr << "ERROR: no se pudo crear el fichero: " << nombreFichero << endl;
        return false;
    }
    imprimirResultados(fout, numDocumentos);
    return true;
}
