#pragma once

#include <iostream>
#include <string>

using namespace std;

#define ISO_8859_SIZE 256
#define DELIMITER_BIT_VEC_SIZE (ISO_8859_SIZE >> 3)               // Total of 256 bits (32 bytes to store ISO-8859 delimiter state machine)
#define AMD64_REGISTER_VEC_SIZE (DELIMITER_BIN_VEC_SIZE >> 3)
#define FILE_LOADER_BUF_SIZE 2048


//TODO: in the future, make 64byte chunk reads, for the moment, byte by byte
struct file_loader {
  char start[FILE_LOADER_BUF_SIZE];
  const char* end;
  char* backpoint;
  char* frontpoint;
  const char* inbuf;
  const char* inbuf_end;
  const char* inbuf_checkpoint;
  const char* inbuf_filename;
  char* outbuf;
  const char* outbuf_end;
  char* outbuf_checkpoint;
  const char* outbuf_filename;

  file_loader();
  bool begin(const char* filename, const char* out_filename);
  void begin(const void* stream, const size_t size);
  void terminate();                     //TODO: unmaps the mapped region (does not zero the buffers) unmaps or nulls the outbuf
  void reload();                        //TODO: displaces the reader by backwriting the data (backpoint to end), moving backpoint and frontpoint
  void full_reload();                   //TODO: fully reloads the buffer placing backpoint and frontpoint at start
  //TODO: displace and reload write remaining data in outbuf, if available

  file_loader& operator=(const file_loader&);
  //TODO: may make operator= for Tokenizador::operator=
};


class Tokenizador
{
  friend ostream& operator<<(ostream&, const Tokenizador&);

public:
  Tokenizador (const string& delimitadoresPalabra, const bool kcasosEspeciales, const bool minuscSinAcentos);	

  Tokenizador (const Tokenizador&);

  Tokenizador ();	

  ~Tokenizador ();

  Tokenizador& operator=(const Tokenizador&);

  void Tokenizar (const string& str, list<string>& tokens) const;

  bool Tokenizar (const string& i, const string& f) const; 

  bool TokenizarListaFicheros (const string& i) const; 

  bool TokenizarDirectorio (const string& i) const; 

  void DelimitadoresPalabra(const string& nuevoDelimiters); 

  void AnyadirDelimitadoresPalabra(const string& nuevoDelimiters);

  string DelimitadoresPalabra() const; 

  void CasosEspeciales (const bool nuevoCasosEspeciales);

  bool CasosEspeciales () const;

  void PasarAminuscSinAcentos (const bool nuevoPasarAminuscSinAcentos);

  bool PasarAminuscSinAcentos() const;

private:
  static const int16_t CAPITAL_START_POINT = 0x41;
  static const int16_t CAPITAL_END_POINT = 0x5a;                                // If curChar  is between CAPITAL_START_POINT and CAPITAL_END_POINT] -> char is capital
  static const int16_t LOWERCASE_START_POINT = 0x61;
  static const int16_t ACCENT_START_POINT = 0xc0;                               // If curChar - ACCENT_START_POINT < 0 -> char has no accent
  static const int16_t ACCENT_REMOVAL_VEC_SIZE = ISO_8859_SIZE - ACCENT_START_POINT;
  static const int16_t TOLOWER_OFFSET = 0x20;

  static const int16_t accentRemovalOffsetVec[ACCENT_REMOVAL_VEC_SIZE] = {
    -0x5f, -0x60, -0x61, -0x62, -0x63, -0x64,       // acctented A -> a
    0x0, 0x0,                                       // weird AE, c trencada
    -0x63, -0x64, -0x65, -0x66,                     // accented E -> e
    -0x63, -0x64, -0x65, -0x66,                     // accented I -> i
    0x0, 0x20,                                      // weird broken D, capital enye -> enye
    -0x63, -0x64, -0x65, -0x66, -0x67,              // accented O -> o
    0x0, 0x0,                                       // x symbol, crossed O
    -0x64, -0x65, -0x66, -0x67,                     // accented U -> u
    0x0, 0x0, 0x0                                   // accented Y, weird flag, beta
    -0x7f, -0x80, -0x81, -0x82, -0x83, -0x84,       // accented a -> a
    0x0, 0x0,                                       // weird ae, c trencada
    -0x83, -0x84, -0x85, -0x86,                     // accented e -> e
    -0x83, -0x84, -0x85, -0x86,                     // accented i -> i
    0x0, 0x0,                                       // weird ro, enye
    -0x83, -0x84, -0x85, -0x86, -0x87               // accented o -> o
    0x0, 0x0,                                       // divide sign, broken o
    -0x84, -0x85, -0x86, -0x87,                     // accented u -> u
    0x0, 0x0, 0x0                                   // accented y, wtf, dieresy y
  };

  uint8_t delimitadoresPalabra[DELIMITER_BIT_VEC_SIZE];
  bool casosEspeciales;
  bool pasarAminuscSinAcentos;
  file_loader loader;

  uint8_t& getDelimiterMemChunk(const char delim);
  bool chekcDelimiter(char) const;
  void setDelimiter(char, bool);
  void resetDelimiters();
  void copyDelimiters(const uint8_t*);
  void copyDelimitersFromString(const string&);
  
  void constructionLogic();

  void normalizeStream(void* data, const size_t sz);
  string_view tokNormalCase(string&);  //TODO
  string_view tokSpecialCase(string&); //TODO
};
