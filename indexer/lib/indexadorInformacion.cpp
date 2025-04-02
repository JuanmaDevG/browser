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


InfTermDoc::~InfTermDoc ()
{
  ft = 0;
  posTerm.clear();
}


InfTermDoc& InformacionTermino::operator= (const InfTermDoc &)
{
}


InfDoc::InfDoc(const InfDoc &)
{
}


InfDoc::InfDoc()
{
}


InfDoc::~InfDoc()
{
}


InfDoc& InfDoc::operator= (const InfDoc &)
{
}


InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs &)
{
}


InfColeccionDocs::InfColeccionDocs()
{
}


InfColeccionDocs::~InfColeccionDocs()
{
}


InfColeccionDocs& InfColeccionDocs::operator=(const InfColeccionDocs &)
{
}


InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta &)
{
}


InformacionTerminoPregunta::InformacionTerminoPregunta()
{
}


InformacionTerminoPregunta::~InformacionTerminoPregunta()
{
}


InformacionTerminoPregunta::InformacionTerminoPregunta& operator= (const InformacionTerminoPregunta &)
{
}


InformacionPregunta::InformacionPregunta(const InformacionPregunta &)
{
}


InformacionPregunta::InformacionPregunta()
{
}


InformacionPregunta::~InformacionPregunta()
{
}


InformacionPregunta::InformacionPregunta& operator= (const InformacionPregunta &)
{
}
