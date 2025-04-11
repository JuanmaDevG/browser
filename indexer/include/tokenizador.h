#pragma once

#include <iostream>
#include <string>
#include <list>
#include <cstdint>
#include <utility>
#include <vector>
#include <ctime>

using namespace std;

#define ISO_8859_SIZE 256
#define DELIMITER_BIT_VEC_SIZE (ISO_8859_SIZE >> 3)               // Total of 256 bits (32 bytes to store ISO-8859 delimiter state machine)
#define MEM_POOL_SIZE 256

struct tm Fecha;

struct io_context {
  union {
    const char* cbegin;
    char* begin;
  };
  union {
    const char* cend;
    char* end;
  };
  union {
    const char* readpoint;
    char* writepoint;
  };
  const char* frontpoint;

  //TODO: io_context functions if needed
};


struct file_loader {
  const char* inbuf;
  const char* inbuf_end;
  size_t inbuf_size;
  const char* readpoint;
  const char* frontpoint;
  int inbuf_fd;

  char* outbuf;
  char* outbuf_end;
  size_t outbuf_size;
  char* writepoint;
  int outbuf_fd;

  file_loader();
  bool begin(const char* in_filename, const char* out_filename);
  void terminate();
  void null_readpoints();
  void null_writepoints();
  bool resize_outfile(const size_t);
  pair<const char*, const char*> getline();
  bool write(const void* buf, const size_t sz);
  bool put(const char);
  Fecha get_mod_date() const; //TODO: return the date from fstat

  void mem_begin(const char* rdbuf, const size_t rdbuf_sz);
  void mem_terminate();
};


struct memory_pool {
  char data[MEM_POOL_SIZE];
  char* buf = data;
  char* buf_end = data + MEM_POOL_SIZE;
  char* writepoint = buf;
  size_t bufsize = MEM_POOL_SIZE;

  void reset();
  bool resize(const size_t);
  bool write(const void* buf, const size_t sz);
  bool put(const char);
};


struct iso_8859_1_bitvec {
  uint8_t data[DELIMITER_BIT_VEC_SIZE];

  bool check(const uint8_t) const;
  void set(uint8_t, bool);
  void reset();
  void copy_from(const iso_8859_1_bitvec&);
  void copy_from(const string&);
  void copy_to(iso_8859_1_bitvec&) const;
};


class IndexadorHash;

class Tokenizador
{
  friend ostream& operator<<(ostream&, const Tokenizador&);
  friend class IndexadorHash;

public:
  Tokenizador (const string& delimitadoresPalabra, const bool kcasosEspeciales, const bool minuscSinAcentos);

  Tokenizador (const Tokenizador&);

  Tokenizador ();	

  ~Tokenizador ();

  Tokenizador& operator=(const Tokenizador&);

  void Tokenizar (const string& str, list<string>& tokens);

  bool Tokenizar (const string& i, const string& f);

  bool TokenizarListaFicheros (const string& i); 

  bool TokenizarDirectorio (const string& i); 

  void DelimitadoresPalabra(const string& nuevoDelimiters); 

  string DelimitadoresPalabra() const;

  void AnyadirDelimitadoresPalabra(const string& nuevoDelimiters);

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

  static const uint8_t NUMERIC_START_POINT = 0x30;
  static const uint8_t NUMERIC_END_POINT = 0x39;

  static constexpr int16_t accentRemovalOffsetVec[ACCENT_REMOVAL_VEC_SIZE] = {
    -0x5f, -0x60, -0x61, -0x62, -0x63, -0x64,       // acctented A -> a
    0x0, 0x0,                                       // weird AE, c trencada
    -0x63, -0x64, -0x65, -0x66,                     // accented E -> e
    -0x63, -0x64, -0x65, -0x66,                     // accented I -> i
    0x0, 0x20,                                      // weird broken D, capital enye -> enye
    -0x63, -0x64, -0x65, -0x66, -0x67,              // accented O -> o
    0x0, 0x0,                                       // x symbol, crossed O
    -0x64, -0x65, -0x66, -0x67,                     // accented U -> u
    0x0, 0x0, 0x0,                                  // accented Y, weird flag, beta
    -0x7f, -0x80, -0x81, -0x82, -0x83, -0x84,       // accented a -> a
    0x0, 0x0,                                       // weird ae, c trencada
    -0x83, -0x84, -0x85, -0x86,                     // accented e -> e
    -0x83, -0x84, -0x85, -0x86,                     // accented i -> i
    0x0, 0x0,                                       // weird ro, enye
    -0x83, -0x84, -0x85, -0x86, -0x87,              // accented o -> o
    0x0, 0x0,                                       // divide sign, broken o
    -0x84, -0x85, -0x86, -0x87,                     // accented u -> u
    0x0, 0x0, 0x0                                   // accented y, wtf, dieresy y
  };

  iso_8859_1_bitvec delimiters;
  bool casosEspeciales;
  bool pasarAminuscSinAcentos;
  file_loader loader;

  pair<const char*, const char*> (Tokenizador::*extractToken)();

  char normalizeChar(const char); //TODO: fix all normalizeChar calls and put conditionals on prenormalization of buffer
  void skipDelimiters(const bool leaveLastOne);
  bool isNumeric(const char) const;
  bool tkFile(const char* ifile, const char* ofile);
  bool tkDirectory(const char* name, const size_t len);

  // Functions for the indexer
  void tkAppend(const string& file, vector<string>& tokens);
  bool tkDirAppend(const string& directory, vector<string>& tokens);

  // Values for extractToken (extract a token from the file_loader inbuf)
  pair<const char*, const char*> extractCommonCaseToken();
  pair<const char*, const char*> extractSpecialCaseToken();

  // Special case detection functions
  const char* multiwordTill();
  const char* urlTill();
  const char* emailTill();
  const char* acronymTill();
  const char* decimalTill();

  void constructionLogic();
  void defaultDelimiters();
  void specialDelimiters();
};
