#pragma once

#include <iostream>
#include <string>
#include <list>
#include <cstdint>
#include <utility>
#include <vector>
#include <bitset>

using namespace std;

#define ISO_8859_SIZE 256
#define DELIMITER_BIT_VEC_SIZE (ISO_8859_SIZE >> 3)


class Tokenizador
{
  friend ostream& operator<<(ostream&, const Tokenizador&);

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
  bitset<256> delimiters;
  bool casosEspeciales;
  bool pasarAminuscSinAcentos;

  extern inline void default_delimiters();
  extern inline void add_delimiters(const unsigned char *restrict p, const size_t n);

  //TODO: look if functions may work
  bool isNumeric(const char) const;
  bool tkFile(const char* ifile, const char* ofile);
  bool tkDirectory(const char* name, const size_t len);

  //TODO: change everything with a state machine (and probably make an enum type)
  // Special case detection functions
  const char* multiwordTill();
  const char* urlTill();
  const char* emailTill();
  const char* acronymTill();
  const char* decimalTill();

  void constructionLogic();
  void defaultDelimiters();
  void specialDelimiters();

  static const uint8_t NUMERIC_START_POINT = 0x30;
  static const uint8_t NUMERIC_END_POINT = 0x39;
  static const unsigned char iso8859_norm_table[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'w', 'x', 'y', 'z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o', 'o', 'o', 0xd7, 'o', 'u', 'u', 'u', 'u', 'y', 0xde, 's',
    'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'o', 'n', 'o', 'o', 'o', 'o', 'o', 0xf7, 'o', 'u', 'u', 'u', 'u', 'y', 0xfe, 'y'
  };
};
