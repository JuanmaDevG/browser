#include "indexadorHash.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

using namespace std;

// TODO: remove this shit, use macros on repeated calls
namespace file_utils {

static bool exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static time_t get_mod_time(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return st.st_mtime;
}

static long get_size(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return (long)st.st_size;
}

} // namespace file_utils

const char *IndexadorHash::indexDefaultFilename = "index.idx";

IndexadorHash &IndexadorHash::operator=(const IndexadorHash &idx) {
  if (this == &idx)
    return *this;
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
  nextId = idx.nextId;
  return *this;
}

IndexadorHash::IndexadorHash()
    : indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""),
      indicePregunta(), infPregunta(), stopWords(), ficheroStopWords(""), tok(),
      directorioIndice(), tipoStemmer(0), stemmer(), almacenarPosTerm(false),
      nextId(1) {}

IndexadorHash::IndexadorHash(const string &fichStopWords,
                             const string &delimitadores, const bool detectComp,
                             const bool minuscSinAcentos,
                             const string &dirIndice, const int tStemmer,
                             const bool almPosTerm)
    : indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""),
      indicePregunta(), infPregunta(), ficheroStopWords(fichStopWords),
      tok(delimitadores, detectComp, minuscSinAcentos),
      directorioIndice(dirIndice), tipoStemmer(tStemmer), stemmer(),
      almacenarPosTerm(almPosTerm), nextId(1) {

  int fd_stw = open(ficheroStopWords.c_str(), O_RDONLY);
  if (fd_stw == -1) {
    cerr << "ERROR: el fichero de stopwords no existe" << endl;
    return;
  }
  struct stat file_info;
  fstat(fd_stw, &file_info);
  unsigned char *const map_stw = (unsigned char *)mmap(
      NULL, file_info.st_size, PROT_READ, MAP_SHARED, fd_stw, 0);
  close(fd_stw);

  if (map_stw == MAP_FAILED) {
    cerr << "ERROR: no hay suficiente memoria para mapear el fichero de "
            "stopwords"
         << endl;
    return;
  }
  madvise(map_stw, file_info.st_size, MADV_SEQUENTIAL | MADV_WILLNEED);
  const unsigned char *const limit = map_stw + file_info.st_size;
  const unsigned char *tk_init = map_stw;
  for (const unsigned char *i = map_stw; i < limit; ++i) {
    if (*i == '\n') {
      if (i == tk_init)
        goto next_word;

      stopWords.emplace(tk_init, i);
    next_word:
      tk_init = i + 1;
      i = tk_init;
    }
  }
  munmap(map_stw, file_info.st_size);
}

IndexadorHash::IndexadorHash(const string &dirIndice)
    : indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""),
      indicePregunta(), infPregunta(), stopWords(), ficheroStopWords(""), tok(),
      directorioIndice(dirIndice), tipoStemmer(0), stemmer(),
      almacenarPosTerm(false), nextId(1) {
  RecuperarIndexacion(dirIndice);
}

IndexadorHash::IndexadorHash(const IndexadorHash &idx)
    : indice(idx.indice), indiceDocs(idx.indiceDocs),
      informacionColeccionDocs(idx.informacionColeccionDocs),
      pregunta(idx.pregunta), indicePregunta(idx.indicePregunta),
      infPregunta(idx.infPregunta), stopWords(idx.stopWords),
      ficheroStopWords(idx.ficheroStopWords), tok(idx.tok),
      directorioIndice(idx.directorioIndice), tipoStemmer(idx.tipoStemmer),
      stemmer(idx.stemmer), almacenarPosTerm(idx.almacenarPosTerm),
      nextId(idx.nextId) {}

IndexadorHash::~IndexadorHash() {}

void IndexadorHash::IndexarDoc(const string &doc_filename,
                               vector<string> &tokens) {
  indexDoc(doc_filename.c_str(), doc_filename.length());
  // TODO: loop that takes tokens
  tokens.clear();
  // TODO: volver a tokenizar en un buffer para rellenar el vector
  // (cambiar incluso la parte publica del tokenizador)

  // ==================================================================
  InfDoc &infDoc = indiceDocs[doc_filename];
  if (infDoc.idDoc > 0) { // Ya estaba indexado
    cerr << "WARNING: el documento " << doc_filename
         << " ya estaba indexado.\n";

    time_t mod_time = file_utils::get_mod_time(doc_filename.c_str());
    if (!(difftime(mod_time, infDoc.fechaModificacion) > 0))
      return;

    // Actualizar fecha y restar contadores globales
    infDoc.fechaModificacion = mod_time;
    informacionColeccionDocs.numTotalPal -= infDoc.numPal;
    infDoc.numPal = 0;
    informacionColeccionDocs.numTotalPalSinParada -= infDoc.numPalSinParada;
    infDoc.numPalSinParada = 0;
    infDoc.numPalDiferentes = 0;
    informacionColeccionDocs.tamBytes -= infDoc.tamBytes;
    infDoc.tamBytes = 0;

    for (auto i = indice.begin(); i != indice.end(); i++) {
      InformacionTermino &it = i->second;
      auto itd_iter = it.l_docs.find(infDoc.idDoc);
      if (itd_iter != it.l_docs.end()) {
        it.ftc -= itd_iter->second.ft;
        it.l_docs.erase(itd_iter);
      }
    }
  } else { // Documento nuevo
    infDoc.idDoc = nextId;
    ++nextId;
    ++informacionColeccionDocs.numDocs;
  }

  list<string> token_list;
  {
    FILE *fp = fopen(doc_filename.c_str(), "rb");
    if (!fp)
      return;
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    string content(fsize, '\0');
    if (fsize > 0)
      fread(&content[0], 1, fsize, fp);
    fclose(fp);
    tok.Tokenizar(content, token_list);
  }

  for (auto &t : token_list)
    if (!t.empty())
      tokens.push_back(t);

  infDoc.fechaModificacion = file_utils::get_mod_time(doc_filename.c_str());
  infDoc.numPal = (int)tokens.size();
  informacionColeccionDocs.numTotalPal += (int)tokens.size();
  infDoc.tamBytes = (int)file_utils::get_size(doc_filename.c_str());
  informacionColeccionDocs.tamBytes += infDoc.tamBytes;

  int posTerm = 0;
  for (auto i = tokens.begin(); i != tokens.end(); i++, ++posTerm) {
    stemmer.stemmer(*i, tipoStemmer);
    if (stopWords.find(*i) != stopWords.end())
      continue;

    ++informacionColeccionDocs.numTotalPalSinParada;
    ++infDoc.numPalSinParada;

    InformacionTermino &infTerm = indice[*i];
    ++infTerm.ftc;
    InfTermDoc &itd = infTerm.l_docs[infDoc.idDoc];
    if (itd.ft == 0)
      ++infDoc.numPalDiferentes;
    ++itd.ft;
    if (almacenarPosTerm)
      itd.posTerm.push_back(posTerm);
  }
}

// TODO: fix this function's quirks
bool IndexadorHash::Indexar(const string &ficheroDocumentos) {
  int fd_docfile = open(ficheroDocumentos.c_str(), O_RDONLY);
  if (fd_docfile == -1) {
    cerr << "ERROR: el fichero de documentos " << ficheroDocumentos
         << " no existe" << endl;
    return false;
  }
  struct stat file_info;
  fstat(fd_docfile, &file_info);
  unsigned char *docfile_map = (unsigned char *)mmap(
      NULL, file_info.st_size, PROT_READ, MAP_SHARED, fd_docfile, 0);
  if (docfile_map == MAP_FAILED) {
    cerr << "ERROR: no hay memoria para mapear el fichero " << ficheroDocumentos
         << endl;
    return false;
  }

  // TODO: modify to avoid using IndexarDoc and so remove vec
  vector<string> tokens;
  const unsigned char *fname_ini = docfile_map;
  const unsigned char *const mbuf_end = docfile_map + file_info.st_size;
  for (const unsigned char *i = docfile_map; i < mbuf_end; ++i) {
    if (*i == '\n') {
      if (fname_ini == i)
        goto next_fname;
      IndexarDoc({fname_ini, i}, tokens);
    next_fname:
      fname_ini = i + 1;
      i = fname_ini;
    }
  }
  return true;
}

bool IndexadorHash::IndexarDirectorio(const string &dirAIndexar) {
  DIR *dir;
  struct dirent *entry;
  string entry_name;
  vector<string> tokens;

  if ((dir = opendir(dirAIndexar.c_str())) == nullptr) {
    cerr << "ERROR: el directorio de indexacion " << dirAIndexar
         << " no existe." << endl;
    return false;
  }

  while ((entry = readdir(dir)) != nullptr) {
    entry_name = entry->d_name;
    if (entry_name == "." || entry_name == "..")
      continue;

    size_t len = entry_name.size();
    if (len >= 3 && entry_name.compare(len - 3, 3, ".tk") == 0)
      continue;

    string full_path = dirAIndexar + "/" + entry_name;

    if (entry->d_type == DT_DIR) {
      IndexarDirectorio(full_path);
    } else {
      IndexarDoc(full_path, tokens);
    }
  }

  closedir(dir);
  informacionColeccionDocs.numTotalPalDiferentes = (int)indice.size();
  return true;
}

bool IndexadorHash::GuardarIndexacion() const {
  if (!directorioIndice.empty()) {
    mkdir(directorioIndice.c_str(), 0755);
  }

  string out_filename(directorioIndice.empty()
                          ? indexDefaultFilename
                          : directorioIndice + "/" + indexDefaultFilename);

  FILE *fp = fopen(out_filename.c_str(), "wb");
  if (!fp) {
    cerr << "ERROR: no se pudo abrir para escritura: " << out_filename << endl;
    return false;
  }

  // Helper lambdas
  auto wbuf = [&](const void *data, size_t n) { fwrite(data, 1, n, fp); };
  auto wsize = [&](size_t v) { fwrite(&v, sizeof(size_t), 1, fp); };
  auto wstr = [&](const string &s) {
    size_t len = s.size();
    fwrite(&len, sizeof(size_t), 1, fp);
    fwrite(s.c_str(), 1, len, fp);
  };
  auto wint = [&](int v) { fwrite(&v, sizeof(int), 1, fp); };

  // informacionColeccionDocs
  wbuf(&informacionColeccionDocs, sizeof(InfColeccionDocs));

  // indiceDocs
  wsize(indiceDocs.size());
  for (auto &kv : indiceDocs) {
    wstr(kv.first);
    wbuf(&kv.second, sizeof(InfDoc));
  }

  // indice
  wsize(indice.size());
  for (auto &kv : indice) {
    wstr(kv.first);
    wint(kv.second.ftc);
    wsize(kv.second.l_docs.size());
    for (auto &dkv : kv.second.l_docs) {
      wint(dkv.first);
      wint(dkv.second.ft);
      wsize(dkv.second.posTerm.size());
      for (int pos : dkv.second.posTerm)
        wint(pos);
    }
  }

  // infPregunta + pregunta
  wbuf(&infPregunta, sizeof(InformacionPregunta));
  wstr(pregunta);

  // indicePregunta
  wsize(indicePregunta.size());
  for (auto &kv : indicePregunta) {
    wstr(kv.first);
    wint(kv.second.ft);
    wsize(kv.second.posTerm.size());
    for (int pos : kv.second.posTerm)
      wint(pos);
  }

  // stopWords
  wsize(stopWords.size());
  for (auto &sw : stopWords)
    wstr(sw);

  wstr(ficheroStopWords);
  wint(tipoStemmer);
  char ap = (char)almacenarPosTerm;
  wbuf(&ap, 1);
  wint(nextId);

  fclose(fp);
  return true;
}

bool IndexadorHash::RecuperarIndexacion(const string &directorioIndexacion) {
  indice.clear();
  indiceDocs.clear();
  informacionColeccionDocs = InfColeccionDocs();
  pregunta = "";
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  stopWords.clear();

  string in_filename(directorioIndexacion.empty()
                         ? indexDefaultFilename
                         : directorioIndexacion + "/" + indexDefaultFilename);

  FILE *fp = fopen(in_filename.c_str(), "rb");
  if (!fp) {
    cerr << "ERROR: no se pudo abrir el índice: " << in_filename << endl;
    return false;
  }

  auto rbuf = [&](void *data, size_t n) { return fread(data, 1, n, fp) == n; };
  auto rsize = [&](size_t &v) { return fread(&v, sizeof(size_t), 1, fp) == 1; };
  auto rstr = [&](string &s) {
    size_t len;
    if (fread(&len, sizeof(size_t), 1, fp) != 1)
      return false;
    s.resize(len);
    return len == 0 || fread(&s[0], 1, len, fp) == len;
  };
  auto rint = [&](int &v) { return fread(&v, sizeof(int), 1, fp) == 1; };

  // informacionColeccionDocs
  if (!rbuf(&informacionColeccionDocs, sizeof(InfColeccionDocs)))
    goto err;

  // indiceDocs
  {
    size_t ndocs;
    if (!rsize(ndocs))
      goto err;
    for (size_t i = 0; i < ndocs; ++i) {
      string name;
      InfDoc doc;
      if (!rstr(name))
        goto err;
      if (!rbuf(&doc, sizeof(InfDoc)))
        goto err;
      indiceDocs[name] = doc;
    }
  }

  // indice
  {
    size_t nterms;
    if (!rsize(nterms))
      goto err;
    for (size_t i = 0; i < nterms; ++i) {
      string term;
      if (!rstr(term))
        goto err;
      InformacionTermino &it = indice[term];
      if (!rint(it.ftc))
        goto err;
      size_t ndocs;
      if (!rsize(ndocs))
        goto err;
      for (size_t j = 0; j < ndocs; ++j) {
        int docId;
        if (!rint(docId))
          goto err;
        InfTermDoc &itd = it.l_docs[docId];
        if (!rint(itd.ft))
          goto err;
        size_t npos;
        if (!rsize(npos))
          goto err;
        for (size_t k = 0; k < npos; ++k) {
          int pos;
          if (!rint(pos))
            goto err;
          itd.posTerm.push_back(pos);
        }
      }
    }
  }

  // infPregunta + pregunta
  if (!rbuf(&infPregunta, sizeof(InformacionPregunta)))
    goto err;
  {
    string preg;
    if (!rstr(preg))
      goto err;
    pregunta = preg;
  }

  // indicePregunta
  {
    size_t nterms;
    if (!rsize(nterms))
      goto err;
    for (size_t i = 0; i < nterms; ++i) {
      string term;
      if (!rstr(term))
        goto err;
      InformacionTerminoPregunta &itp = indicePregunta[term];
      if (!rint(itp.ft))
        goto err;
      size_t npos;
      if (!rsize(npos))
        goto err;
      for (size_t k = 0; k < npos; ++k) {
        int pos;
        if (!rint(pos))
          goto err;
        itp.posTerm.push_back(pos);
      }
    }
  }

  // stopWords
  {
    size_t nsw;
    if (!rsize(nsw))
      goto err;
    for (size_t i = 0; i < nsw; ++i) {
      string sw;
      if (!rstr(sw))
        goto err;
      stopWords.insert(sw);
    }
  }

  {
    string fsw;
    if (!rstr(fsw))
      goto err;
    ficheroStopWords = fsw;
    if (!rint(tipoStemmer))
      goto err;
    char ap;
    if (!rbuf(&ap, 1))
      goto err;
    almacenarPosTerm = (bool)ap;
    if (!rint(nextId))
      goto err;
  }

  directorioIndice = directorioIndexacion;
  fclose(fp);
  return true;

err:
  cerr << "ERROR: fichero de índice corrupto o incompleto: " << in_filename
       << endl;
  fclose(fp);
  indice.clear();
  indiceDocs.clear();
  return false;
}

bool IndexadorHash::IndexarPregunta(const string &preg) {
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = preg;

  list<string> token_list;
  tok.Tokenizar(preg, token_list);
  token_list.remove_if([](const string &s) { return s.empty(); });

  if (token_list.empty()) {
    cerr << "ERROR: la pregunta no contiene ningún término." << endl;
    return false;
  }

  infPregunta.numTotalPal = (int)token_list.size();

  int posTerm = 0;
  for (auto &t : token_list) {
    string term = t;
    stemmer.stemmer(term, tipoStemmer);

    if (stopWords.find(term) != stopWords.end()) {
      ++posTerm;
      continue;
    }

    ++infPregunta.numTotalPalSinParada;

    InformacionTerminoPregunta &itp = indicePregunta[term];
    if (itp.ft == 0)
      ++infPregunta.numTotalPalDiferentes;
    ++itp.ft;
    if (almacenarPosTerm)
      itp.posTerm.push_back(posTerm);
    ++posTerm;
  }

  if (indicePregunta.empty()) {
    cerr << "ERROR: la pregunta no contiene ningún término con contenido."
         << endl;
    return false;
  }
  return true;
}

bool IndexadorHash::DevuelvePregunta(string &preg) const {
  if (indicePregunta.empty())
    return false;
  preg = pregunta;
  return true;
}

bool IndexadorHash::DevuelvePregunta(const string &word,
                                     InformacionTerminoPregunta &inf) {
  list<string> tmp;
  tok.Tokenizar(word, tmp);
  tmp.remove_if([](const string &s) { return s.empty(); });
  string term = tmp.empty() ? word : tmp.front();
  auto it = indicePregunta.find(term);
  if (it == indicePregunta.end()) {
    inf = InformacionTerminoPregunta();
    return false;
  }
  inf = it->second;
  return true;
}

bool IndexadorHash::DevuelvePregunta(InformacionPregunta &inf) const {
  if (indicePregunta.empty()) {
    inf = InformacionPregunta();
    return false;
  }
  inf = infPregunta;
  return true;
}

bool IndexadorHash::Devuelve(const string &word, InformacionTermino &inf) {
  list<string> tmp;
  tok.Tokenizar(word, tmp);
  tmp.remove_if([](const string &s) { return s.empty(); });
  string term = tmp.empty() ? word : tmp.front();
  auto it = indice.find(term);
  if (it == indice.end()) {
    inf = InformacionTermino();
    return false;
  }
  inf = it->second;
  return true;
}

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) {
  list<string> tmp;
  tok.Tokenizar(word, tmp);
  tmp.remove_if([](const string &s) { return s.empty(); });
  string term = tmp.empty() ? word : tmp.front();
  auto it = indice.find(term);
  if (it == indice.end()) {
    infDoc = InfTermDoc();
    return false;
  }
  auto dit = indiceDocs.find(nomDoc);
  if (dit == indiceDocs.end()) {
    infDoc = InfTermDoc();
    return false;
  }
  auto itd = it->second.l_docs.find(dit->second.idDoc);
  if (itd == it->second.l_docs.end()) {
    infDoc = InfTermDoc();
    return false;
  }
  infDoc = itd->second;
  return true;
}

bool IndexadorHash::Existe(const string &word) {
  list<string> tmp;
  tok.Tokenizar(word, tmp);
  tmp.remove_if([](const string &s) { return s.empty(); });
  string term = tmp.empty() ? word : tmp.front();
  return indice.find(term) != indice.end();
}

bool IndexadorHash::BorraDoc(const string &nomDoc) {
  auto dit = indiceDocs.find(nomDoc);
  if (dit == indiceDocs.end())
    return false;

  InfDoc &infDoc = dit->second;

  informacionColeccionDocs.numTotalPal -= infDoc.numPal;
  informacionColeccionDocs.numTotalPalSinParada -= infDoc.numPalSinParada;
  informacionColeccionDocs.tamBytes -= infDoc.tamBytes;
  --informacionColeccionDocs.numDocs;

  // Eliminar el documento de cada término
  for (auto &kv : indice) {
    InformacionTermino &it = kv.second;
    auto itd = it.l_docs.find(infDoc.idDoc);
    if (itd != it.l_docs.end()) {
      it.ftc -= itd->second.ft;
      it.l_docs.erase(itd);
    }
  }

  // Corregir términos huerfanos que no tienen lista de docs
  for (auto i = indice.begin(); i != indice.end();) {
    if (i->second.l_docs.empty())
      i = indice.erase(i);
    else
      ++i;
  }

  indiceDocs.erase(dit);
  informacionColeccionDocs.numTotalPalDiferentes = (int)indice.size();
  return true;
}

void IndexadorHash::VaciarIndiceDocs() {
  indice.clear();
  indiceDocs.clear();
  informacionColeccionDocs = InfColeccionDocs();
  nextId = 1;
}

void IndexadorHash::VaciarIndicePreg() {
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = "";
}

int IndexadorHash::NumPalIndexadas() const { return (int)indice.size(); }

string IndexadorHash::DevolverFichPalParada() const { return ficheroStopWords; }

void IndexadorHash::ListarPalParada() const {
  for (auto &sw : stopWords)
    cout << sw << '\n';
}

int IndexadorHash::NumPalParada() const { return (int)stopWords.size(); }

string IndexadorHash::DevolverDelimitadores() const {
  return tok.DelimitadoresPalabra();
}

bool IndexadorHash::DevolverCasosEspeciales() const {
  return tok.CasosEspeciales();
}

bool IndexadorHash::DevolverPasarAminuscSinAcentos() const {
  return tok.PasarAminuscSinAcentos();
}

bool IndexadorHash::DevolverAlmacenarPosTerm() const {
  return almacenarPosTerm;
}

string IndexadorHash::DevolverDirIndice() const { return directorioIndice; }

int IndexadorHash::DevolverTipoStemming() const { return tipoStemmer; }

void IndexadorHash::ListarInfColeccDocs() const {
  cout << informacionColeccionDocs << endl;
}

void IndexadorHash::ListarTerminos() const {
  for (auto &kv : indice)
    cout << kv.first << '\t' << kv.second << endl;
}

bool IndexadorHash::ListarTerminos(const string &nomDoc) const {
  auto dit = indiceDocs.find(nomDoc);
  if (dit == indiceDocs.end())
    return false;

  int docId = dit->second.idDoc;
  for (auto &kv : indice) {
    auto itd = kv.second.l_docs.find(docId);
    if (itd != kv.second.l_docs.end())
      cout << kv.first << '\t' << kv.second << endl;
  }
  return true;
}

void IndexadorHash::ListarDocs() const {
  for (auto &kv : indiceDocs)
    cout << kv.first << '\t' << kv.second << endl;
}

bool IndexadorHash::ListarDocs(const string &nomDoc) const {
  auto dit = indiceDocs.find(nomDoc);
  if (dit == indiceDocs.end())
    return false;
  cout << dit->first << '\t' << dit->second << endl;
  return true;
}

void IndexadorHash::indexDoc(const char *fname, const size_t fname_len) {
  int fd_doc = open(fname, O_RDONLY);
  if (fd_doc == -1) {
    cerr << "ERROR: el documento a indexar " << fname
         << " no se ha podido abrir" << endl;
    return;
  }
  struct stat file_info;
  fstat(fd_doc, &file_info);
  unsigned char *file_map = (unsigned char *)mmap(
      NULL, file_info.st_size, PROT_READ, MAP_SHARED, fd_doc, 0);
  if (file_map == MAP_FAILED) {
    cerr << "ERROR: el fichero a indexar " << fname << " no existe" << endl;
    return;
  }
  close(fd_doc);

  // TODO: bring the indexation work here

  munmap(file_map, file_info.st_size);
  return;
}
