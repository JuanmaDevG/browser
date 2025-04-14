#pragma once

#include <unordered_map>
#include <list> //TODO: probably remove (cause of mem fragmentation)


using namespace std;

class InfTermDoc;
class IndexadorHash;

class InformacionTermino { 
  friend ostream& operator<<(ostream& s, const InformacionTermino& p);
  friend class IndexadorHash;
public:
  InformacionTermino(const InformacionTermino &);
  InformacionTermino();
  ~InformacionTermino();
  InformacionTermino& operator=(const InformacionTermino &);
private:
  int ftc;
  unordered_map<int, InfTermDoc> l_docs; 
};


class InfTermDoc {
  friend ostream& operator<<(ostream& s, const InfTermDoc& p);
  friend class IndexadorHash;
public:
  InfTermDoc(const InfTermDoc &);
  InfTermDoc();
  ~InfTermDoc();
  InfTermDoc& operator= (const InfTermDoc &);
private:
  int ft;
  list<int> posTerm;
};


class InfDoc {
  friend ostream& operator<<(ostream& s, const InfDoc& p);
  friend class IndexadorHash;
public:
  InfDoc(const int idDoc);
  InfDoc(const InfDoc &);
  InfDoc();	
  ~InfDoc();
  InfDoc & operator= (const InfDoc &);
private:
  int idDoc;
  int numPal;
  int numPalSinParada;
  int numPalDiferentes; // No stopwords
  int tamBytes;
  Fecha fechaModificacion;
};


class InfColeccionDocs { 
  friend ostream& operator<<(ostream& s, const InfColeccionDocs& p);
  friend class IndexadorHash;
public:
  InfColeccionDocs (const InfColeccionDocs &);
  InfColeccionDocs ();
  ~InfColeccionDocs ();
  InfColeccionDocs & operator= (const InfColeccionDocs &);
private:
  int numDocs;
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;  // No stopwords
  int tamBytes;               // All collection
};


class InformacionTerminoPregunta { 
  friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p);
  friend class IndexadorHash;
public:
  InformacionTerminoPregunta (const InformacionTerminoPregunta &);
  InformacionTerminoPregunta ();
  ~InformacionTerminoPregunta ();
  InformacionTerminoPregunta & operator= (const InformacionTerminoPregunta &);
private:
  int ft;	// Frecuencia total del término en la pregunta
  list<int> posTerm;	
  // Solo se almacenará esta información si el campo privado del indexador almacenarPosTerm == true
  // Lista de números de palabra en los que aparece el término en la pregunta. Los números de palabra comenzarán desde cero (la primera palabra de la pregunta). Se numerarán las palabras de parada. Estará ordenada de menor a mayor posición.
};


class InformacionPregunta {
  friend ostream& operator<<(ostream& s, const InformacionPregunta& p);
  friend class IndexadorHash;
public:
  InformacionPregunta(const InformacionPregunta &);
  InformacionPregunta();
  ~InformacionPregunta();
  InformacionPregunta & operator=(const InformacionPregunta &);
private:
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
};


ostream& operator<<(ostream& s, const InfTermDoc& p) {
  s << "ft: " << p.ft;
  for(auto i = p.posTerm.cbegin(); i != p.posterm.cend(); i++)
    s << '\t' << *i;

  return s;
}


ostream& operator<<(ostream& s, const InfDoc& p) {
  s << "idDoc: " << p.idDoc << "\tnumPal: " << p.numPal << "\tnumPalSinParada: " << p.numPalSinParada << "\tnumPalDiferentes: " << p.numPalDiferentes << "\ttamBytes: " << p.tamBytes;

  return s;
}


ostream& operator<<(ostream&, const InformacionPregunta&);
  s << "numTotalPal: " << p.numTotalPal << "\tnumTotalPalSinParada: "<< p.numTotalPalSinParada << "\tnumTotalPalDiferentes: " << numTotalPalDiferentes;

  return s;
}


ostream& operator<<(ostream& s, const InformacionTermino& p) {
  s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
  for(auto& doc : p.l_docs)
    s << "\tId.Doc: " << doc.first << '\t' << doc.second;

  return s;
}


ostream& operator<<(ostream&, const InfColeccionDocs& p) {
  s << "numDocs: " << p.numDocs << "\tnumTotalPal: " << p.numTotalPal << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada << "\tnumTotalPalDiferentes: " << numTotalPalDiferentes << "\ttamBytes: " << p.tamBytes;

  return s;
}


ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
  s << "ft: " << p.ft;
  for(auto i = p.posTerm.cbegin(); i != p.posTerm.cend(); i++)
    s << '\t' << *i;

  return s;
}
