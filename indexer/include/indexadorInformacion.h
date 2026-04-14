#pragma once

#include <list>
#include <tokenizador.h>
#include <unordered_map>

using namespace std;

class InfTermDoc;
class IndexadorHash;

class InformacionTermino {
  friend ostream &operator<<(ostream &s, const InformacionTermino &p);
  friend class IndexadorHash;

public:
  InformacionTermino(const InformacionTermino &);
  InformacionTermino();
  ~InformacionTermino();
  InformacionTermino &operator=(const InformacionTermino &);
  InfTermDoc &operator=(const InfTermDoc &itd);

private:
  int ftc;
  unordered_map<int, InfTermDoc> l_docs;
};

class InfTermDoc {
  friend ostream &operator<<(ostream &s, const InfTermDoc &p);
  friend class IndexadorHash;
  friend class InformacionTermino;

public:
  InfTermDoc(const InfTermDoc &);
  InfTermDoc();
  ~InfTermDoc();
  InfTermDoc &operator=(const InfTermDoc &);

private:
  int ft;
  list<int> posTerm;
};

class InfDoc {
  friend ostream &operator<<(ostream &s, const InfDoc &p);
  friend class IndexadorHash;

public:
  InfDoc(const int idDoc);
  InfDoc(const InfDoc &);
  InfDoc();
  ~InfDoc();
  InfDoc &operator=(const InfDoc &);

private:
  int idDoc;
  int numPal;
  int numPalSinParada;
  int numPalDiferentes; // No stopwords
  int tamBytes;
  time_t fechaModificacion;
};

class InfColeccionDocs {
  friend ostream &operator<<(ostream &s, const InfColeccionDocs &p);
  friend class IndexadorHash;

public:
  InfColeccionDocs(const InfColeccionDocs &);
  InfColeccionDocs();
  ~InfColeccionDocs();
  InfColeccionDocs &operator=(const InfColeccionDocs &);

private:
  int numDocs;
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes; // No stopwords
  int tamBytes;              // All collection
};

class InformacionTerminoPregunta {
  friend ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p);
  friend class IndexadorHash;

public:
  InformacionTerminoPregunta(const InformacionTerminoPregunta &);
  InformacionTerminoPregunta();
  ~InformacionTerminoPregunta();
  InformacionTerminoPregunta &operator=(const InformacionTerminoPregunta &);

private:
  int ft;
  list<int> posTerm;
  // Solo se almacenará esta información si el campo privado del indexador
  // almacenarPosTerm == true Lista de números de palabra en los que aparece el
  // término en la pregunta. Los números de palabra comenzarán desde cero (la
  // primera palabra de la pregunta). Se numerarán las palabras de parada.
  // Estará ordenada de menor a mayor posición.
};

class InformacionPregunta {
  friend ostream &operator<<(ostream &s, const InformacionPregunta &p);
  friend class IndexadorHash;

public:
  InformacionPregunta(const InformacionPregunta &);
  InformacionPregunta();
  ~InformacionPregunta();
  InformacionPregunta &operator=(const InformacionPregunta &);

private:
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
};

ostream &operator<<(ostream &s, const InfTermDoc &p);
ostream &operator<<(ostream &s, const InfDoc &p);
ostream &operator<<(ostream &os, const InformacionPregunta &p);
ostream &operator<<(ostream &s, const InformacionTermino &p);
ostream &operator<<(ostream &os, const InfColeccionDocs &p);
ostream &operator<<(ostream &os, const InformacionTerminoPregunta &p);
