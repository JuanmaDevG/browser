#include <tokenizador.h>


file_reader::file_reader() : end(start + FILE_READER_BUF_SIZE), backpoint(start), frontpoint(start), rdbuf(nullptr), rdbuf_size(0), outbuf(nullptr) {}


void file_reader::begin(const char* filename, const char* output_filename)
{
}


void file_reader::end()
{
}


void file_reader::begin(const void* stream, const size_t size, char *const output_stream)
{
}


void file_reader::end_stream()
{
  rdbuf = nullptr;
  rdbuf_size = 0;
  backpoint = start;
  frontpoint = start;
  output_stream = nullptr;
}


void file_reader::displace()
{
}


void file_reader::reload()
{
}


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


Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool casosEspeciales, const bool minuscSinAcentos) :
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos), file_reader()
{
  constructionLogic();
  copyDelimitersFromString(delimitadoresPalabra);
}


Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.minuscSinAcentos), file_reader()
{
  constructionLogic();
  copyDelimiters(tk.delimitadoresPalabra);
}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false), file_reader()
{
  static const char* delimDefaults = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  char const* i = delimDefaults;
  constructionLogic();
  while(*i != '\0')
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
  //TODO: substitute string as buffer to uint64_t chunk that can cycleReload
  string buffer(str);
  tokens.clear();
  if(pasarAminuscSinAcentos)
    normalizeStream(buffer);

  auto scanner = str.cbegin();
  //TODO: remember to use string::assign to copy raw data inside a string buffer
  //TODO: decide between tokNormalCases() and tokSpecialCases
}


bool Tokenizador::Tokenizar(const string& i, const string& f) const
{
  //TODO: open file -> mmap -> copy to buffer -> normalize -> tokenize -> if not enough info then do partial load
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


void Tokenizador::CasosEspeciales(const bool nuevoCasosEspeciales)
{
  casosEspeciales = nuevoCasosEspeciales;
}


void Tokenizador::CasosEspeciales() const
{
  return casosEspeciales;
}


void Tokenizador::PasarAminuscSinAcentos(const bool nuevoPasarAminuscSinAcentos)
{
  pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
}


bool Tokenizador::PasarAminuscSinAcentos() const
{
  return pasarAminuscSinAcentos;
}


extern inline uint8_t& getDelimiterMemChunk(const char delim)
{
  return this->delimitadoresPalabra[reinterpret_cast<uint8_t>(x) >> 3];
}


extern inline bool Tokenizador::checkDelimiter(const char delim_idx) const
{
  return (getDelimiterMemChunk(delim_idx) >> (delim_idx & 0b111)) & 1;
}


extern inline void Tokenizador::setDelimiter(const char delim_idx, const bool val)
{
  if(val)
    getDelimiterMemChunk(delim_idx) = getDelimiterMemChunk(delim_idx) | (1 << (reinterpret_cast<uint8_t>(delim_idx) & 0b111));
  else
    getDelimiterMemChunk(delim_idx) = getDelimiterMemChunk(delim_idx) & (0xff - (1 << (reinterpret_cast<uint8_t>(delim_idx) & 0b111)));
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
  resetDelimiters();
  if(casosEspeciales)
    setDelimiter(0x20 /*empty space*/, true);
}


//TODO: probably delete this function in favour of file_reader functions
void Tokenizador::normalizeStream(string& buffer)
{
  int16_t curChar;
  for(auto i = buffer.begin(); i != buffer.end(); i++)
  {
    curChar = static_cast<int16_t>(*i);
    if(curChar >= CAPITAL_START_POINT && curChar <= CAPITAL_END_POINT)
      curChar += Tokenizador::TOLOWER_OFFSET;
    else if(curChar - ACCENT_START_POINT >= 0)
      curChar += Tokenizador::accentRemovalOffsetVec[curChar - ACCENT_START_POINT];
    
    *i = static_cast<char>(curChar);
  }
}
