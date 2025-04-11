#include "indexadorHash.h"

#include <string.h>

using namespace std;

IndexadorHash::IndexadorHash() : indice(), indiceDocs(), informacionColeccionDocs(), indicePregunta(), infPregunta(), stopWords(), 
  tok(), tipoStemmer(0), stemmer(), pregunta(""), ficheroStopWords(""), directorioIndice(""), tipoStemmer(0), almacenarPosTerm(false) {}

IndexadorHash::IndexadorHash(const string& fichStopWords, const string& delimitadores, const bool detectComp, const bool minuscSinAcentos, const string& dirIndice, const int tStemmer, const bool almPosTerm) :
  indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""), indicePregunta(), infPregunta(), ficheroStopWords(fichStopWords), tok(delimitadores, detectComp, minuscSinAcentos), directorioIndice(dirIndice), 
  tipoStemmer(tStemmer), stemmer(), almacenarPosTerm(almPosTerm)
{
  if(!tok.loader.begin(ficheroStopWords))
  {
    cerr << "ERROR: el fichero de stopwords " << ficheroStopWords << " no existe" << endl;
    ficheroStopWords = "";
    return;
  }

  stopWords.reserve(tok.loader.inbuf_size >> 3);
  auto reader = tok.loader.getline();
  while(reader.first)
  {
    stopWords.emplace(reader.first, reader.second);
    reader = tok.loader.getline();
  }
  tok.loader.terminate();
}


IndexadorHash::~IndexadorHash() {}


IndexadorHash& IndexadorHash::operator=(const IndexadorHash& idx)
{
  indice = idx.indice;
  indiceDocs = idx.indiceDocs;
  informacionColeccionDocs = idx.informacionColeccionDocs;
  pregunta = idx.pregunta;
  indicePregunta = idx.indicePregunta;
  infPregunta = idx.infPregunta;
  stopWords = idx.stopWords;
  ficheroStopWords = idx.ficheroStopWords;
  tok = idx.tok;
  directorioIndice = idx.directorioIndice;
  tipoStemmer = idx.tipoStemmer;
  stemmer = idx.stemmer;
  almacenarPosTerm = idx.almacenarPosTerm;
}


bool IndexadorHash::Indexar(const string& ficheroDocumentos)
{
  file_loader fl;
  if(!fl.begin(ficheroDocumentos.c_str()))
  {
    cerr << "ERROR: el fichero de documentos " << ficheroDocumentos << " no existe" << endl;
    return false;
  }
  vector<string> tokens;
  auto line = fl.getline();
  string cur_file(line.first, line.second);

  while(line.first)
  {
    tok.tkAppend(cur_file, tokens);
    for(auto i = tokens.begin(); i != tokens.end(); i++)
      stemmer.stemmer(*i, tipoStemmer);

    //Finished tokenizing this shit
    line = fl.getline();
    cur_file.clear();
    cur_file.assign(line.first, line.second);
    tokens.clear();
  }

  fl.terminate();
}


bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar)
{
}


bool IndexadorHash::GuardarIndexacion() const
{
}


bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion)
{
}


bool IndexadorHash::IndexarPregunta(const string& preg)
{
}


bool IndexadorHash::DevuelvePregunta(string& preg) const
{
}


bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const
{
}


bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const
{
}


bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const
{
}


bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& InfDoc) const
{
}


bool IndexadorHash::Existe(const string& word) const
{
}


bool IndexadorHash::BorraDoc(const string& nomDoc)
{
}


void IndexadorHash::VaciarIndiceDocs()
{
}


void IndexadorHash::VaciarIndicePreg()
{
}


int IndexadorHash::NumPalIndexadas() const
{
}


string IndexadorHash::DevolverFichPalParada () const
{
}


void IndexadorHash::ListarPalParada() const
{
}


int IndexadorHash::NumPalParada() const
{
}


string IndexadorHash::DevolverDelimitadores () const
{
}


bool IndexadorHash::DevolverCasosEspeciales () const
{
}


bool IndexadorHash::DevolverPasarAminuscSinAcentos () const
{
}


bool IndexadorHash::DevolverAlmacenarPosTerm () const
{
}


string IndexadorHash::DevolverDirIndice () const
{
}


int IndexadorHash::DevolverTipoStemming () const
{
}


void IndexadorHash::ListarInfColeccDocs() const
{
}


void IndexadorHash::ListarTerminos() const
{
}


bool IndexadorHash::ListarTerminos(const string& nomDoc) const
{
}


void IndexadorHash::ListarDocs() const
{
}


bool IndexadorHash::ListarDocs(const string& nomDoc) const
{
}
