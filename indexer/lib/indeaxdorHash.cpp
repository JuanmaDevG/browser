#include "indexadorHash.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

using namespace std;

IndexadorHash::IndexadorHash() : indice(), indiceDocs(), informacionColeccionDocs(), indicePregunta(), infPregunta(), stopWords(), 
  tok(), tipoStemmer(0), stemmer(), pregunta(""), ficheroStopWords(""), directorioIndice(""), tipoStemmer(0), almacenarPosTerm(false),
  nextId(1) {}


void IndexadorHash::IndexarDoc(const string& doc_filename, vector<string>& tokens)
{
  tokens.clear();
  if(!file_loader::exists(doc_filename.c_str()))
    return;

  InfDoc& infDoc = indiceDocs[doc_filename]; 
  if(infDoc.idDoc > 0) // Was previously inidexed
  {
    cerr << "WARNING: el documento " << doc_filename << " ya estaba indexado.\n";
    Fecha mod_date = file_loader::get_mod_date(doc_filename.c_str());
    if(!(difftime(mktime(mod_date), mktime(infDoc.fechaModificacion)) > 0))
      return;

    // Erasing each term information
    infDoc.fechaModificacion = mod_date;
    informacionColeccionDocs.numTotalPal -= infDoc.numPal;
    infDoc.numPal = 0;
    informacionColeccionDocs.numTotalPalSinParada -= infDoc.numPalSinParada;
    infDoc.numPalSinParada = 0;
    infDoc.numPalDiferentes = 0;
    informacionColeccionDocs.tamBytes -= infDoc.tamBytes;
    infDoc.tamBytes = 0;
    for(auto i = indice.begin(); i != indice.end(); i++)
    {
      InformacionTermino& it = i->second;
      auto itd_iter = it.l_docs.find(infDoc.idDoc);
      if(itd_iter != it.l_docs.end())
      {
        it.ftc -= itd_iter->second.ft;
        it.l_docs.remove(itd_iter);
      }
    }
  }
  else // New document
  {
    infDoc.idDoc = nextId;
    ++nextId;
    ++informacionColeccionDocs.numDocs;
  }

  if(!tok.tkAppend(doc_filename, tokens)) return;

  infDoc.fechaModificacion = file_loader::get_mod_date(doc_filename.c_str());
  infDoc.numPal = tokens.size();
  informacionColeccionDocs.numTotalPal += tokens.size();
  infDoc.numPal = tokens.size();
  infDoc.tamBytes = file_loader::get_size(doc_filename.c_str());
  informacionColeccionDocs.tamBytes += infDoc.tamBytes;

  int posTerm = 0;
  for(auto i = tokens.begin(); i != tokens.end(); i++, ++posTerm)
  {
    stemmer.stemmer(*i, tipoStemmer);
    if(stopWords.find(*i) != stopWords.end())
      continue;

    ++informacionColeccionDocs.numTotalPalSinParada;
    ++infDoc.numPalSinParada;

    InformacionTermino& infTerm = indice[*i];
    //if(infTerm.ftc == 0) ++informacionColeccionDocs.numTotalPalDiferentes;

    ++infTerm.ftc;
    InfTermDoc& itd = infTerm.l_docs[infDoc.idDoc];
    if(itd.ft == 0)
      ++infDoc.numPalDiferentes;
    ++itd.ft;
    if(almacenarPosTerm)
      itd.posTerm.push_back(posTerm);
  }
}

IndexadorHash::IndexadorHash(const string& fichStopWords, const string& delimitadores, const bool detectComp, const bool minuscSinAcentos, const string& dirIndice, const int tStemmer, const bool almPosTerm) :
  indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""), indicePregunta(), infPregunta(), ficheroStopWords(fichStopWords), tok(delimitadores, detectComp, minuscSinAcentos), directorioIndice(dirIndice), 
  tipoStemmer(tStemmer), stemmer(), almacenarPosTerm(almPosTerm), nextId(1)
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
    string& token = *stopWords.emplace(reader.first, reader.second).first;
    stemmer.stemmer(token, tipostemmer);
    reader = tok.loader.getline();
  }
  tok.loader.terminate();
}


IndexadorHash::IndexadorHash(const string& dirIndice) : directorioIndice(dirIndice)
{
  RecuperarIndexacion(dirIndice);
}


IndexadorHash::IndexadorHash(const IndexadorHash& idx) : indice(idx.indice), indiceDocs(idx.indiceDocs),
  informacionColeccionDocs(idx.informacionColeccionDocs), indicePregunta(idx.indicePregunta), infPregunta(idx.infPregunta),
  stopWords(idx.stopWords), tok(idx.tok), tipoStemmer(idx.tipoStemmer), stemmer(idx.stemmer), pregunta(idx.pregunta),
  ficheroStopWords(idx.ficheroStopWords), directorioIndice(idx.directorioIndice), tipoStemmer(idx.tipoStemmer), stemmer(),
  almacenarPosTerm(idx.almacenarPosTerm), nextId(idx.nextId) {}


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
  string doc_filename;

  while(line.first)
  {
    doc_filename.clear();
    doc_filename.assign(line.first, line.second);

    IndexarDoc(doc_filename, tokens);

    line = fl.getline();
  }
  informacionColeccionDocs.numTotalPalDiferentes = indice.size();

  cerr << flush;
  fl.terminate();
  return true;
}


bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar)
{
  DIR* dir;
  struct dirent* entry;
  string entry_name;
  vector<string> tokens;

  if((dir = opendir(dirAIndexar.c_str())) == nullptr)
  {
    cerr << "ERROR: el directorio de indexacion " << dirAIndexar << " no existe." << endl;
    return false;
  }

  while((entry = readdir(dir)) != nullptr)
  {
    entry_name = entry->d_name;
    if(entry_name == "." || entry_name == ".." || entry_name.find(".tk", entry_name.length() - 3, 3))
        continue;

    if(entry->d_type == DT_DIR)
    {
      IndexarDirectorio(dirAIndexar + "/" + entry_name);
    }
    else
    {
      IndexarDoc(entry_name, tokens);
    }
  }

  return true;
}


bool IndexadorHash::GuardarIndexacion() const
{
  string out_filename((directorioIndice == "" ? indexDefaultFilename : directorioIndice + "/" + indexDefaultFilename));
  file_loader fl(nullptr, out_filename.c_str());
  size_t doc_size = 0;

  // Doc name lengths
  for(auto i = indiceDocs.cbegin(); i != indiceDocs.cend(); i++)
    doc_size += i->first.length();

  // Term lengths + l_docs metadata + termPos list lengths * sizeof(int)
  for(auto i = indice.cbegin(); i != indice.cend(); i++)
  {
    doc_size += 
      i->first.length() 
      + (i->second.l_docs.size() * (sizeof(int) + sizeof(int) + sizeof(uint64_t)));
    for(auto j = i->second.l_docs.cbegin(); j != i->second.l_docs.cend(); j++)
      doc_size += j->second.posTerm.size() << 2;
  }

  // Query term lengths + posTerm list lengths * sizeof(int)
  for(auto i = indicePregunta.cbegin(); i != indicePregunta.cend(); i++)
    doc_size += i->first.length() + (i->second.posTerm.size() << 2);

  // Stopword lengths
  for(auto i = stopwords.cbegin(); i != stopwords.cend(); i++)
    doc_size += i->length();

  doc_size +=
    sizeof(InfColeccionDocs)
    + sizeof(size_t) + (indiceDocs.size() * (sizeof(uint64_t) + sizeof(InfDoc)))                      // indiceDocs
    + sizeof(size_t) + (indice.size() * (sizeof(uint64_t) + sizeof(int) + sizeof(uint64_t)))          // indice
    + sizeof(InformacionPregunta)
    + sizeof(size_t) + pregunta.length()                                                              // pregunta
    + sizeof(size_t) + (indicePregunta.size() * (sizeof(uint64_t) + sizeof(int) + sizeof(uint64_t)))  // indicePregunta
    + sizeof(size_t) + (stopWords.size() * sizeof(uint64_t))                                          // stopWords
    + sizeof(size_t) + ficheroStopWords.length() + sizeof(int) + 1 + sizeof(int);

  fl.resize_outfile(doc_size);
  
  // informacionColeccionDocs
  fl.write(&informacionColeccionDocs, sizeof(InfColeccionDocs));

  // indiceDocs
  fl.write(&indiceDocs.size(), sizeof(size_t));
  for(auto i = indiceDocs.cbegin(); i != indiceDocs.cend(); i++)
  {
    fl.write(&i->first.length(), sizeof(size_t));
    fl.write(i->first.c_str(), i->first.length());
    fl.write(&i->second, sizeof(InfDoc));
  }

  // indice
  fl.write(&indice.size(), sizeof(size_t));
  for(auto i = indice.cbegin(); i != indice.cend(); i++)
  {
    fl.write(&i->first.length(), sizeof(size_t));
    fl.write(i->first.c_str(), i->first.length());
    fl.write(&i->second.ftc, sizeof(int));
    fl.write(&i->second.l_docs.size(), sizeof(size_t));
    for(auto j = i->second.l_docs.cbegin(); j != i->second.l_docs.cbegin(); j++)
    {
      fl.write(&j->first, sizeof(int));
      fl.write(&j->second.ft, sizeof(int));
      fl.write(&j->second.posTerm.size(), sizeof(size_t));
      for(auto k = j->second.posTerm.cbegin(); k != j->second.posTerm.cend(); k++)
        fl.write(&(*k), sizeof(int));
    }
  }
  fl.write(&infPregunta, sizeof(InformacionPregunta));
  fl.write(&pregunta.length(), sizeof(size_t));
  fl.write(pregunta.c_str(), pregunta.length());

  // indicePregunta
  fl.write(&indicePregunta.size(), sizeof(size_t));
  for(auto i = indicePregunta.cbegin(); i != indicePregunta.cend(); i++)
  {
    fl.write(&i->first.length(), sizeof(size_t));
    fl.write(i->first.c_str(), i->first.length());
    fl.write(&i->second.ft, sizeof(int));
    fl.write(&i->second.posTerm.size(), sizeof(size_t));
    for(auto j = i->second.posTerm.cbegin(); j != i->second.posTerm.cend(); j++)
      fl.write(&(*j), sizeof(int));
  }

  // stopWords
  fl.write(&stopWords.size(), sizeof(size_t));
  for(auto i = stopWords.cbegin(); i != stopWords.cend(); i++)
  {
    fl.write(&i->length(), sizeof(size_t));
    fl.write(i->c_str(), i->length());
  }
  
  fl.write(&ficheroStopWords.length(), sizeof(size_t));
  fl.write(ficheroStopWords.c_str(), ficheroStopWords.length());
  fl.write(&tipoStemmer, sizeof(int));
  fl.put((char)almPosTerm);
  fl.write(&nextId, sizeof(int));

  fl.terminate();
}


bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion)
{
  // TODO: clean data structures
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
