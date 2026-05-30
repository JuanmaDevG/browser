#pragma once

#include "indexadorHash.h"
#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <unordered_map>

using namespace std;

class ResultadoRI {
    friend ostream& operator<<(ostream& os, const ResultadoRI& res) {
        os << res.vSimilitud << "\t\t" << res.idDoc << "\t" << res.numPregunta << endl;
        return os;
    }
public:
    ResultadoRI(const double& kvSimilitud, const long int& kidDoc, const int& np)
        : vSimilitud(kvSimilitud), idDoc(kidDoc), numPregunta(np) {}

    double   VSimilitud()  const { return vSimilitud;  }
    long int IdDoc()       const { return idDoc;        }
    int      NumPregunta() const { return numPregunta;  }

    bool operator<(const ResultadoRI& lhs) const {
        if (numPregunta == lhs.numPregunta)
            return (vSimilitud < lhs.vSimilitud);
        else
            return (numPregunta > lhs.numPregunta);
    }

private:
    double   vSimilitud;
    long int idDoc;
    int      numPregunta;
};


class Buscador : public IndexadorHash {

    friend ostream& operator<<(ostream& s, const Buscador& p) {
        string preg;
        s << "Buscador: " << endl;
        if (const_cast<Buscador&>(p).DevuelvePregunta(preg))
            s << "\tPregunta indexada: " << preg << endl;
        else
            s << "\tNo hay ninguna pregunta indexada" << endl;
        s << "\tDatos del indexador: " << endl << (IndexadorHash)p;
        return s;
    }

public:
    Buscador(const string& directorioIndexacion, const int& f);
    Buscador(const Buscador&);
    ~Buscador();
    Buscador& operator=(const Buscador&);

    bool Buscar(const int& numDocumentos = 99999);
    bool Buscar(const string& dirPreguntas, const int& numDocumentos,
                const int& numPregInicio, const int& numPregFin);

    void ImprimirResultadoBusqueda(const int& numDocumentos = 99999) const;
    bool ImprimirResultadoBusqueda(const int& numDocumentos,
                                   const string& nombreFichero) const;

    int  DevolverFormulaSimilitud() const;
    bool CambiarFormulaSimilitud(const int& f);

    void   CambiarParametrosDFR(const double& kc);
    double DevolverParametrosDFR() const;

    void CambiarParametrosBM25(const double& kk1, const double& kb);
    void DevolverParametrosBM25(double& kk1, double& kb) const;

private:
    Buscador();

    struct ColData { int N; double avr_ld; };
    ColData getColData() const;

    void buildDocMaps(unordered_map<int,int>& idToLen,
                      unordered_map<int,string>& idToName) const;

    unordered_map<string,int> getTermsQuery() const;

    bool buscarDFR(const int& numDocumentos, const int numPregunta);
    bool buscarBM25(const int& numDocumentos, const int numPregunta);

    void imprimirResultados(ostream& out, const int& numDocumentos) const;

    priority_queue<ResultadoRI> docsOrdenados;
    int    formSimilitud;
    double c;
    double k1;
    double b;
};
