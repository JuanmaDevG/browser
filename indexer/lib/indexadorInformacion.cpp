#include "indexadorInformacion.h"

InformacionTermino::InformacionTermino(const InformacionTermino &it)
    : ftc(it.ftc), l_docs(it.l_docs) {}

InformacionTermino::InformacionTermino() : ftc(0), l_docs() {}

InformacionTermino::~InformacionTermino() {
  ftc = 0;
  l_docs.clear();
}

InfTermDoc::InfTermDoc(const InfTermDoc &itd)
    : ft(itd.ft), posTerm(itd.posTerm) {}

InfTermDoc::InfTermDoc() : ft(0), posTerm() {}

InfTermDoc::~InfTermDoc() {
  ft = 0;
  posTerm.clear();
}

InfTermDoc &InfTermDoc::operator=(const InfTermDoc &itd) {
  ft = itd.ft;
  posTerm = itd.posTerm;
  return *this;
}

InformacionTermino &
InformacionTermino::operator=(const InformacionTermino &itd) {
  ftc = itd.ftc;
  l_docs = itd.l_docs;
  return *this;
}

InfDoc::InfDoc(const int idDoc)
    : idDoc(idDoc), numPal(0), numPalSinParada(0), numPalDiferentes(0),
      tamBytes(0), fechaModificacion() {}

InfDoc::InfDoc(const InfDoc &infd)
    : idDoc(infd.idDoc), numPal(infd.numPal),
      numPalSinParada(infd.numPalSinParada), numPalDiferentes(0),
      tamBytes(infd.tamBytes), fechaModificacion() {}

InfDoc::InfDoc()
    : idDoc(0), numPal(0), numPalSinParada(0), numPalDiferentes(0), tamBytes(0),
      fechaModificacion(0) {}

InfDoc::~InfDoc() {}

InfDoc &InfDoc::operator=(const InfDoc &infd) {
  idDoc = infd.idDoc;
  numPal = infd.numPal;
  numPalSinParada = infd.numPalSinParada;
  tamBytes = infd.tamBytes;
  fechaModificacion = infd.fechaModificacion;
  numPalDiferentes = infd.numPalDiferentes;
  return *this;
}

InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs &icd)
    : numDocs(icd.numDocs), numTotalPal(icd.numTotalPal),
      numTotalPalSinParada(icd.numTotalPalSinParada),
      numTotalPalDiferentes(icd.numTotalPalDiferentes), tamBytes(icd.tamBytes) {
}

InfColeccionDocs::InfColeccionDocs()
    : numDocs(0), numTotalPal(0), numTotalPalSinParada(0),
      numTotalPalDiferentes(0), tamBytes(0) {}

InfColeccionDocs::~InfColeccionDocs() {}

InfColeccionDocs &InfColeccionDocs::operator=(const InfColeccionDocs &icd) {
  numDocs = icd.numDocs;
  numTotalPal = icd.numTotalPal;
  numTotalPalSinParada = icd.numTotalPalSinParada;
  numTotalPalDiferentes = icd.numTotalPalDiferentes;
  tamBytes = icd.tamBytes;
  return *this;
}

InformacionTerminoPregunta::InformacionTerminoPregunta(
    const InformacionTerminoPregunta &itp)
    : ft(itp.ft), posTerm(itp.posTerm) {}

InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(0), posTerm() {}

InformacionTerminoPregunta::~InformacionTerminoPregunta() {
  ft = 0;
  posTerm.clear();
}

InformacionTerminoPregunta &
InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta &itp) {
  ft = itp.ft;
  posTerm = itp.posTerm;
  return *this;
}

InformacionPregunta::InformacionPregunta(const InformacionPregunta &ip)
    : numTotalPal(ip.numTotalPal),
      numTotalPalSinParada(ip.numTotalPalSinParada),
      numTotalPalDiferentes(ip.numTotalPalDiferentes) {}

InformacionPregunta::InformacionPregunta()
    : numTotalPal(0), numTotalPalSinParada(0), numTotalPalDiferentes(0) {}

InformacionPregunta::~InformacionPregunta() {}

InformacionPregunta &
InformacionPregunta::operator=(const InformacionPregunta &ip) {
  numTotalPal = ip.numTotalPal;
  numTotalPalSinParada = ip.numTotalPalSinParada;
  numTotalPalDiferentes = ip.numTotalPalDiferentes;
  return *this;
}

ostream &operator<<(ostream &s, const InfTermDoc &p) {
  s << "ft: " << p.ft;
  for (auto i = p.posTerm.cbegin(); i != p.posTerm.cend(); i++)
    s << '\t' << *i;

  return s;
}

ostream &operator<<(ostream &s, const InfDoc &p) {
  s << "idDoc: " << p.idDoc << "\tnumPal: " << p.numPal
    << "\tnumPalSinParada: " << p.numPalSinParada
    << "\tnumPalDiferentes: " << p.numPalDiferentes
    << "\ttamBytes: " << p.tamBytes;

  return s;
}

ostream &operator<<(ostream &os, const InformacionPregunta &p) {
  os << "numTotalPal: " << p.numTotalPal
     << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
     << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;

  return os;
}

ostream &operator<<(ostream &s, const InformacionTermino &p) {
  s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
  for (auto &doc : p.l_docs)
    s << "\tId.Doc: " << doc.first << '\t' << doc.second;

  return s;
}

ostream &operator<<(ostream &os, const InfColeccionDocs &p) {
  os << "numDocs: " << p.numDocs << "\tnumTotalPal: " << p.numTotalPal
     << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
     << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes
     << "\ttamBytes: " << p.tamBytes;

  return os;
}

ostream &operator<<(ostream &os, const InformacionTerminoPregunta &p) {
  os << "ft: " << p.ft;
  for (auto i = p.posTerm.cbegin(); i != p.posTerm.cend(); i++)
    os << '\t' << *i;

  return os;
}
