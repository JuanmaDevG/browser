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
  // TODO: El primer documento indexado en la colecci�n ser� el identificador 1
  int numPal;	// N� total de palabras del documento
  int numPalSinParada;
  int numPalDiferentes;	//TODO: recuerda no parada
  int tamBytes;
  Fecha fechaModificacion;
  // Atributo correspondiente a la fecha y hora (completa) de modificaci�n del documento. El tipo "Fecha/hora" lo elegir�/implementar� el alumno
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
  int numDocs;	// N� total de documentos en la colecci�n
  int numTotalPal;
  // N� total de palabras en la colecci�n
  int numTotalPalSinParada;
  // N� total de palabras sin stop-words en la colecci�n 
  int numTotalPalDiferentes;	
  // N� total de palabras diferentes en la colecci�n que no sean stop-words (sin acumular la frecuencia de cada una de ellas)
  int tamBytes;	// Tama�o total en bytes de la colecci�n
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
  int ft;	// Frecuencia total del t�rmino en la pregunta
  list<int> posTerm;	
  // Solo se almacenar� esta informaci�n si el campo privado del indexador almacenarPosTerm == true
  // Lista de n�meros de palabra en los que aparece el t�rmino en la pregunta. Los n�meros de palabra comenzar�n desde cero (la primera palabra de la pregunta). Se numerar�n las palabras de parada. Estar� ordenada de menor a mayor posici�n.
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
