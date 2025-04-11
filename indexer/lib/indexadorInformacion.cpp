#include "indexadorInformacion.h"


InformacionTermino::InformacionTermino(const InformacionTermino & it) : ftc(it.ftc), l_docs(it.l_docs) {}


InformacionTermino::InformacionTermino() : ftc(0), l_docs() {}


InformacionTermino::~InformacionTermino()
{
  ftc = 0;
  l_docs.clear();
}


InformacionTermino& InformacionTermino::operator=(const InformacionTermino& it)
{
  ftc = it.ftc;
  l_docs = it.l_docs;
  return *this;
}


InfTermDoc::InfTermDoc(const InfTermDoc& itd) : ft(itd.ft), posTerm(itd.posTerm) {}


InfTermDoc::InfTermDoc() : ft(0), posTerm() {}


InfTermDoc::~InfTermDoc()
{
  ft = 0;
  posTerm.clear();
}


InfTermDoc& InformacionTermino::operator=(const InfTermDoc & itd)
{
  ft = itd.ft;
  posTerm = itd.posTerm;
  return *this;
}


InfDoc::InfDoc(const InfDoc & infd) : idDoc(infd.idDoc), numPal(infd.numPal), numPalSinParada(infd.numPalSinParada), 
  tambytes(infd.tamBytes), fechaModificacion(infd.fechaModificacion) {}


InfDoc::InfDoc() : idDoc(0), numPal(0), numPalSinParada(0), tambytes(0) fechaModificacion({0, 0, 0, 0, 0, 0}) {}


InfDoc::~InfDoc() {}


InfDoc& InfDoc::operator= (const InfDoc & infd)
{
  idDoc = infd.idDoc;
  numPal = infd.numPal;
  numPalSinParada = infd.numPalSinParada;
  tambytes = infd.tambytes;
  fechaModificacion = infd.fechaModificacion;
  return *this;
}


InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs & icd) : numDocs(icd.numDocs), numTotalPal(icd.numTotalPal),
  numTotalPalSinParada(icd.numTotalPalSinParada), numTotalPalDiferentes(icd.numTotalPalDiferentes), tambytes(icd.tambytes) 
{}


InfColeccionDocs::InfColeccionDocs() : numDocs(0), numTotalPal(0), numtotalPalSinParada(0), numTotalPalDiferentes(0), 
  tambytes(0) {}


InfColeccionDocs::~InfColeccionDocs() {}


InfColeccionDocs& InfColeccionDocs::operator=(const InfColeccionDocs& icd)
{
  numDocs = icd.numDocs;
  numTotalPal = icd.numTotalPal;
  numTotalPalSinParada = icd.numTotalPalSinParada;
  numTotalPalDiferentes = icd.numTotalPalDiferentes;
  tambytes = icd.tambytes;
  return *this;
}


InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta & itp) : ft(itp.ft), posTerm(itp.posTerm)
{}


InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(0), posTerm() {}


InformacionTerminoPregunta::~InformacionTerminoPregunta()
{
  ft = 0;
  posterm.clear();
}


InformacionTerminoPregunta::InformacionTerminoPregunta& operator= (const InformacionTerminoPregunta & itp)
{
  ft = itp.ft;
  posTerm = itp.posTerm;
  return *this;
}


InformacionPregunta::InformacionPregunta(const InformacionPregunta& ip) : numTotalPal(ip.numTotalPal), 
  numTotalPalSinParada(ip.numTotalPalSinParada), numTotalPalDiferentes(ip.numTotalPalDiferentes) {}


InformacionPregunta::InformacionPregunta() : numTotalPal(0), numTotalPalSinParada(0), numTotalPalDiferentes(0) {}


InformacionPregunta::~InformacionPregunta() {}


InformacionPregunta::InformacionPregunta& operator=(const InformacionPregunta& ip)
{
  numTotalPal = ip.numTotalPal;
  numtotalPalSinParada = ip.numTotalPalSinParada;
  numTotalPalDiferentes = ip.numTotalPalDiferentes;
  return *this;
}
