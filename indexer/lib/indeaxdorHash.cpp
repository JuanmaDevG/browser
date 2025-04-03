#include "indexadorHash.h"
#include "indexadorInformacion.h"
#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

// Constructor por defecto de IndexadorHash
IndexadorHash::IndexadorHash() {
    // Inicialización básica
    informacionIndexada.clear(); // Suponiendo que es una estructura de datos tipo unordered_map
    stopWords.clear(); // Inicialización de lista de palabras vacías
    delimitadores = " ,.;:!?()[]\n\t"; // Delimitadores estándar
    indexadorInfo = nullptr; // Suponiendo que se usa un puntero a indexadorInformacion

    cout << "IndexadorHash creado correctamente." << endl;
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
