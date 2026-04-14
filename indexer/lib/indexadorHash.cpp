#include "indexadorHash.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

using namespace std;

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

// ---------------------------------------------------------------------------

const char *IndexadorHash::indexDefaultFilename = "index.idx";

IndexadorHash::IndexadorHash()
    : indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""),
      indicePregunta(), infPregunta(), stopWords(),
      ficheroStopWords(indexDefaultFilename), tok(), directorioIndice(),
      stemmer(), tipoStemmer(0), almacenarPosTerm(false), nextId(1) {}

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

  FILE *fp = fopen(ficheroStopWords.c_str(), "r");
  if (!fp) {
    cerr << "ERROR: el fichero de stopwords " << ficheroStopWords
         << " no existe" << endl;
    this->ficheroStopWords = "";
    return;
  }

  char line[4096];
  while (fgets(line, sizeof(line), fp)) {
    size_t len = strlen(line);
    // Quitar salto de línea
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';
    if (len == 0)
      continue;

    string token(line, len);
    // BUG 2: el original usaba "tipostemmer" (nombre incorrecto)
    stemmer.stemmer(token, tipoStemmer);
    stopWords.insert(token);
  }
  fclose(fp);
}

IndexadorHash::IndexadorHash(const string &dirIndice)
    // BUG 3: el original no inicializaba los miembros antes de llamar a
    // RecuperarIndexacion; se inicializan aquí por seguridad.
    : indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""),
      indicePregunta(), infPregunta(), stopWords(), ficheroStopWords(""), tok(),
      directorioIndice(dirIndice), stemmer(), tipoStemmer(0),
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

void IndexadorHash::IndexarDoc(const string &doc_filename,
                               vector<string> &tokens) {
  tokens.clear();

  // BUG 6: el original usaba file_loader::exists que no está definido.
  if (!file_utils::exists(doc_filename.c_str()))
    return;

  InfDoc &infDoc = indiceDocs[doc_filename];
  if (infDoc.idDoc > 0) { // Ya estaba indexado
    cerr << "WARNING: el documento " << doc_filename
         << " ya estaba indexado.\n";

    // BUG 7: el original usaba file_loader::get_mod_date devolviendo Fecha
    // (struct tm *) y difftime(mktime(...)) — pero InfDoc::fechaModificacion
    // es time_t. Se trabaja directamente con time_t.
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
        // BUG 8: el original creaba una unordered_map sin uso alguno aquí.
        // Eliminado.
      }
    }
  } else { // Documento nuevo
    infDoc.idDoc = nextId;
    ++nextId;
    ++informacionColeccionDocs.numDocs;
  }

  // BUG 9: el original llamaba a tok.tkAppend que no existe en el Tokenizador.
  // La tokenización se hace mediante Tokenizar(string, list<string>) y luego
  // se pasa a vector.
  list<string> token_list;
  tok.Tokenizar(doc_filename, token_list);
  // Nota: Tokenizar abre el fichero internamente (versión de dos args escribe
  // a .tk). Usamos la versión en memoria leyendo el fichero nosotros.
  // Como el tokenizador sólo expone Tokenizar(string str, list<string>&)
  // para tokenizar texto en memoria, leemos el fichero manualmente.
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

bool IndexadorHash::Indexar(const string &ficheroDocumentos) {
  // BUG 10: el original usaba file_loader que no está definido.
  FILE *fp = fopen(ficheroDocumentos.c_str(), "r");
  if (!fp) {
    cerr << "ERROR: el fichero de documentos " << ficheroDocumentos
         << " no existe" << endl;
    return false;
  }

  vector<string> tokens;
  char line[4096];
  while (fgets(line, sizeof(line), fp)) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';
    if (len == 0)
      continue;

    string doc_filename(line, len);
    IndexarDoc(doc_filename, tokens);
  }

  informacionColeccionDocs.numTotalPalDiferentes = (int)indice.size();
  cerr << flush;
  fclose(fp);
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

// ---------------------------------------------------------------------------
// IndexarPregunta
// ---------------------------------------------------------------------------
bool IndexadorHash::IndexarPregunta(const string &preg) {
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = preg;

  list<string> token_list;
  tok.Tokenizar(preg, token_list);

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

// ---------------------------------------------------------------------------
// DevuelvePregunta  (3 sobrecargas)
// ---------------------------------------------------------------------------
bool IndexadorHash::DevuelvePregunta(string &preg) const {
  if (indicePregunta.empty())
    return false;
  preg = pregunta;
  return true;
}

bool IndexadorHash::DevuelvePregunta(const string &word,
                                     InformacionTerminoPregunta &inf) const {
  string term = word;
  stemmer.stemmer(term, tipoStemmer);
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

// ---------------------------------------------------------------------------
// Devuelve  (2 sobrecargas)
// ---------------------------------------------------------------------------
bool IndexadorHash::Devuelve(const string &word,
                             InformacionTermino &inf) const {
  string term = word;
  stemmer.stemmer(term, tipoStemmer);
  auto it = indice.find(term);
  if (it == indice.end()) {
    inf = InformacionTermino();
    return false;
  }
  inf = it->second;
  return true;
}

bool IndexadorHash::Devuelve(const string &word, const string &nomDoc,
                             InfTermDoc &infDoc) const {
  string term = word;
  stemmer.stemmer(term, tipoStemmer);
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

// ---------------------------------------------------------------------------
// Existe
// ---------------------------------------------------------------------------
bool IndexadorHash::Existe(const string &word) const {
  string term = word;
  stemmer.stemmer(term, tipoStemmer);
  return indice.find(term) != indice.end();
}

// ---------------------------------------------------------------------------
// BorraDoc
// ---------------------------------------------------------------------------
bool IndexadorHash::BorraDoc(const string &nomDoc) {
  auto dit = indiceDocs.find(nomDoc);
  if (dit == indiceDocs.end())
    return false;

  InfDoc &infDoc = dit->second;

  // Restar contadores globales
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

  indiceDocs.erase(dit);
  informacionColeccionDocs.numTotalPalDiferentes = (int)indice.size();
  return true;
}

// ---------------------------------------------------------------------------
// VaciarIndiceDocs
// ---------------------------------------------------------------------------
void IndexadorHash::VaciarIndiceDocs() {
  indice.clear();
  indiceDocs.clear();
  informacionColeccionDocs = InfColeccionDocs();
  nextId = 1;
}

// ---------------------------------------------------------------------------
// VaciarIndicePreg
// ---------------------------------------------------------------------------
void IndexadorHash::VaciarIndicePreg() {
  indicePregunta.clear();
  infPregunta = InformacionPregunta();
  pregunta = "";
}

// ---------------------------------------------------------------------------
// Getters simples
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Listar*
// ---------------------------------------------------------------------------
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
