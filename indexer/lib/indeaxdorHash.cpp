#include "indexadorHash.h"

#include <string.h>

using namespace std;

IndexadorHash::IndexadorHash() : indice(), indiceDocs(), informacionColeccionDocs(), indicePregunta(), infPregunta(), stopWords(), 
  tok(), tipoStemmer(0), pregunta(""), ficheroStopWords(""), directorioIndice(""), tipoStemmer(0), almacenarPosTerm(false) {}

IndexadorHash::IndexadorHash(const string& fichStopWords, const string& delimitadores, const bool detectComp, const bool minuscSinAcentos, const string& dirIndice, const int tStemmer, const bool almPosTerm) :
  indice(), indiceDocs(), informacionColeccionDocs(), pregunta(""), indicePregunta(), infPregunta(), ficheroStopWords(fichStopWords), tok(delimitadores, detectComp, minuscSinAcentos), directorioIndice(dirIndice), 
  tipoStemmer(tStemmer), almacenarPosTerm(almPosTerm)
{
  if(!tok.loader.begin(ficheroStopWords))
  {
    cerr << "ERROR: el fichero de stopwords " << ficheroStopWords << " no existe" << endl;
    ficheroStopWords = "";
    return;
  }

  stopWords.reserve(tok.loader.inbuf_size >> 3);
  auto reader = tok.loader.getline(0);
  while(reader.first)
  {
    stopWords.emplace(reader.first, reader.second);
    reader = tok.loader.getline((reader.second - reader.first) +1);
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
  pair<const char*, const char*> line = fl.getline();

  while(line.first)
  {
    //TODO: indexation logic functions
  }

  fl.terminate();
}


bool IndexadorHash::IndexarFichero(const string& rutaArchivo) {
    int fd = open(rutaArchivo.c_str(), O_RDONLY);
    if (fd == -1) return false;

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) == -1) {
        close(fd);
        return false;
    }

    size_t fileSize = fileInfo.st_size;
    vector<char> buffer(fileSize);
    
    if (read(fd, buffer.data(), fileSize) != static_cast<ssize_t>(fileSize)) {
        close(fd);
        return false;
    }
    close(fd);

    string contenido(buffer.begin(), buffer.end());

    vector<string> tokens;
    tokenizador.Tokenizar(contenido, tokens);

    for (const auto& palabra : tokens) {
        if (stopWords.find(palabra) == stopWords.end())
            informacionIndexada[palabra]++;
    }

    return true;
}


void IndexadorHash::VaciarIndice() {
    informacionIndexada.clear();
}


bool IndexadorHash::GuardarIndice(const string& rutaArchivo) {
    int fd = open(rutaArchivo.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) return false;

    for (const auto& [palabra, frecuencia] : informacionIndexada) {
        string linea = palabra + " " + to_string(frecuencia) + "\n";
        if (write(fd, linea.c_str(), linea.size()) == -1) {
            close(fd);
            return false;
        }
    }

    close(fd);
    return true;
}


bool IndexadorHash::CargarStopWords(const string& rutaArchivo) {
    int fd = open(rutaArchivo.c_str(), O_RDONLY);
    if (fd == -1) return false;

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) == -1) {
        close(fd);
        return false;
    }

    size_t fileSize = fileInfo.st_size;
    vector<char> buffer(fileSize);

    if (read(fd, buffer.data(), fileSize) != static_cast<ssize_t>(fileSize)) {
        close(fd);
        return false;
    }
    close(fd);

    string contenido(buffer.begin(), buffer.end());
    vector<string> tokens;
    tokenizador.Tokenizar(contenido, tokens);

    for (const auto& palabra : tokens)
        stopWords.insert(palabra);

    return true;
}


void IndexadorHash::ConfigurarDelimitadores(const string& nuevosDelimitadores) {
    delimitadores = nuevosDelimitadores;
    tokenizador.SetDelimitadores(nuevosDelimitadores);
}
