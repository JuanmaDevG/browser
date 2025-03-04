#include <tokenizador.h>

//TODO: cleanup header file

ostream& operator<<(ostream& os, const Tokenizador& tk)
{
  cout << "DELIMITADORES: ";
  for(uint8_t i=0; i < ISO_8859_SIZE; ++i)
    if(tk.checkDelimiter(reinterpret_cast<char>(i)))
      cout << reinterpret_cast<char>(i);

  cout << " TRATA DE CASOS ESPECIALES: " << tk.casosEspeciales 
    << " PASAR A MINUSCULAS Y SIN ACENTOS: " << tk.pasarAminuscSinAcentos
    << flush;
}

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& casosEspeciales, const bool& minuscSinAcentos) :
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos)
{
  constructionLogic();
  copyDelimitersFromString(delimitadoresPalabra);
}

Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.minuscSinAcentos)
{
  constructionLogic();
  copyDelimiters(tk.delimitadoresPalabra);
}

Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false)
{
  static const char* delimDefaults = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  char const* i = delimDefaults;
  constructionLogic();
  while(i != '\0')
  {
    setDelimiter(*i, true);
    ++i;
  }
}	

Tokenizador::~Tokenizador()
{
  resetDelimiters();
}

Tokenizador::Tokenizador& operator=(const Tokenizador& tk)
{
  casosEspeciales = tk.casosEspeciales;
  pasarAminuscSinAcentos = tk.pasarAminuscSinAcentos;
  copyDelimiters(tk.delimitadoresPalabra);
}

void Tokenizador::Tokenizar(const string& str, list<string>& tokens) const
{
  tokens.clear();
  auto scanner = str.cbegin();
  //TODO: tokenizing a string as data buffer may be an entirely different function from memory mapping files
}

bool Tokenizador::Tokenizar(const string& i, const string& f) const
{
} 

bool Tokenizador::TokenizarListaFicheros(const string& i) const
{
} 

bool Tokenizador::TokenizarDirectorio(const string& i) const
{
} 

void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters)
{
  resetDelimiters();
  copyDelimitersFromString(nuevoDelimiters);
} 

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{
  copyDelimitersFromString(nuevoDelimiters);
}

string Tokenizador::DelimitadoresPalabra() const
{
  string result;
  result.reserve(ISO_8859_SIZE);
  for(int i=0; i < ISO_8859_SIZE; ++i)
  {
    if(checkDelimiter(const_cast<char>(i)))
      result.push_back(const_cast<char>(i));
  }

  return result;
} 

void Tokenizador::CasosEspeciales(const bool& nuevoCasosEspeciales)
{
  casosEspeciales = nuevoCasosEspeciales;
}

void Tokenizador::CasosEspeciales() const
{
  return casosEspeciales;
}

void Tokenizador::PasarAminuscSinAcentos(const bool& nuevoPasarAminuscSinAcentos)
{
  pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
}

bool Tokenizador::PasarAminuscSinAcentos() const
{
  return pasarAminuscSinAcentos;
}

extern inline bool Tokenizador::checkDelimiter(const char delim_idx) const
{
  return (get_chunk(delim_idx) >> (delim_idx & 0b111)) & 1;
}

extern inline void Tokenizador::setDelimiter(const char delim_idx, const bool val)
{
  if(val)
    get_chunk(delim_idx) = get_chunk(delim_idx) | (1 << (delim_idx & 0b111));
  else
    get_chunk(delim_idx) = get_chunk(delim_idx) & (0xff - (1 << (delim_idx & 0b111)));
}

extern inline void Tokenizador::resetDelimiters()
{
  int64_t* d = reinterpret_cast<int64_t*>(delimitadoresPalabra);
  const int64_t* end = reinterpret_cast<int64_t*>(delimitadoresPalabra) + AMD64_REGISTER_VEC_SIZE;
  while(d < end)
  {
    *d = 0;
    ++d;
  }
}

extern inline void Tokenizador::copyDelimiters(const uint8_t* delim)
{
  int64_t const* rdata = reinterpret_cast<int64_t*>(delim);
  int64_t* buffer = reinterpret_cast<int64_t*>(delimitadoresPalabra);
  const int64_t* end = reinterpret_cast<int64_t*>(delimitadoresPalabra) + AMD64_REGISTER_VEC_SIZE;
  while(buffer < end)
  {
    *buffer = *rdata;
    ++rdata;
    ++buffer;
  }
}

extern inline void copyDelimitersFromString(const string& delimitadoresPalabra)
{
  for(auto reader = delimitadoresPalabra.cbegin(); reader != delimitadoresPalabra.cend(); reader++)
    setDelimiter(*reader, true);
}


extern inline void Tokenizador::constructionLogic()
{
  if(casosEspeciales)
    setDelimiter(' ', true);
}
