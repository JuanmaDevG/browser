#pragma once

#include <unordered_map>
#include <list> //TODO: probably remove (cause of mem fragmentation)


using namespace std;

class InfTermDoc;
class IndexadorHash;

class InformacionTermino { 
  friend ostream& operator<<(ostream& s, const InformacionTermino& p);
  friend class IndexadorInformacion;
public:
  InformacionTermino (const InformacionTermino &);
  InformacionTermino ();
  ~InformacionTermino ();
  InformacionTermino& operator= (const InformacionTermino &);
private:
  int ftc;
  unordered_map<int, InfTermDoc> l_docs; 
};


class InfTermDoc {
  friend ostream& operator<<(ostream& s, const InfTermDoc& p);
  friend class IndexadorInformacion;
public:
  InfTermDoc (const InfTermDoc &);
  InfTermDoc ();
  ~InfTermDoc ();
  InfTermDoc& operator= (const InfTermDoc &);
private:
  int ft;
  list<int> posTerm;	
};


class InfDoc { 
  friend ostream& operator<<(ostream& s, const InfDoc& p);
  friend class IndexadorInformacion;
public:
  InfDoc (const InfDoc &);
  InfDoc ();	
  ~InfDoc ();
  InfDoc & operator= (const InfDoc &);
private:
  int idDoc;
  // TODO: El primer documento indexado en la colección será el identificador 1
  int numPal;	// Nº total de palabras del documento
  int numPalSinParada;
  int numPalDiferentes;	//TODO: recuerda no parada
  int tamBytes;
  Fecha fechaModificacion;
  // Atributo correspondiente a la fecha y hora (completa) de modificación del documento. El tipo "Fecha/hora" lo elegirá/implementará el alumno
};


class InfColeccionDocs { 
  friend ostream& operator<<(ostream& s, const InfColeccionDocs& p);
  friend class IndexadorInformacion;
public:
  InfColeccionDocs (const InfColeccionDocs &);
  InfColeccionDocs ();
  ~InfColeccionDocs ();
  InfColeccionDocs & operator= (const InfColeccionDocs &);
private:
  int numDocs;	// Nº total de documentos en la colección
  int numTotalPal;
  // Nº total de palabras en la colección
  int numTotalPalSinParada;
  // Nº total de palabras sin stop-words en la colección 
  int numTotalPalDiferentes;	
  // Nº total de palabras diferentes en la colección que no sean stop-words (sin acumular la frecuencia de cada una de ellas)
  int tamBytes;	// Tamaño total en bytes de la colección
};


class InformacionTerminoPregunta { 
  friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p);
  friend class IndexadorInformacion;
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
  friend class IndexadorInformacion;
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
