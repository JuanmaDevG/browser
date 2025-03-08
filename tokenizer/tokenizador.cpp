#include <tokenizador.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


bool file_loader::begin(const char* filename, const char* output_filename = nullptr)
{
  int fd = open(filename, O_RDONLY);
  stat in_fileinfo;
  if(fd < 0)
    return false;

  fstat(fd, &in_fileinfo);
  inbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  inbuf_end = inbuf + in_fileinfo.st_size;
  backpoint = inbuf;
  frontpoint = inbuf;
  inbuf_filename = filename;

  if(!output_filename) return true;
  fd = open(output_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if(fd < 0)
  {
    munmap(inbuf, in_fileinfo.st_size);
    null_readpoints();
    return false;
  }
  ftruncate(fd, in_fileinfo.st_size);
  outbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  outbuf_end = outbuf + in_fileinfo.st_size;
  outbuf_writepoint = outbuf;
  outbuf_filename = output_filename;

  return true;
}


void file_loader::terminate()
{
  munmap(inbuf, static_cast<size_t>(inbuf_end - inbuf));

  if(outbuf) {
    munmap(outbuf, static_cast<size_t>(outbuf_end - outbuf));
    if(outbuf_end - outbuf_writepoint)  // Adjusting the filesize to the bytes written
    {
      int fd = open(output_filename, O_WRONLY);
      ftruncate(fd, (outbuf_end - outbuf) - (outbuf_end - outbuf_writepoint));
    }
  }

  null_readpoints();
  null_writepoints();
}


extern inline void file_loader::null_readpoints()
{
  inbuf = nullptr;
  inbuf_end = nullptr;
  readpoint = nullptr;
}


extern inline void file_reader::null_writepoints()
{
  outbuf = nullptr;
  outbuf_end = nullptr;
  writepoint = nullptr;
  outbuf_filename = nullptr;
}


void file_reader::grow_outfile(size_t how_much)
{
    off_t writer_offset = outbuf_writepoint - outbuf;
    munmap(outbuf, static_cast<size_t>(outbuf_end - outbuf));
    how_much += static_cast<size_t>(outbuf_end - outbuf);

    int fd = open(outbuf_filename, O_WRONLY);
    ftruncate(fd, how_much);
    outbuf = mmap(nullptr, how_much, PROT_WRITE, MAP_SHARED, fd, 0);
    outbuf_end = outbuf + how_much;
    outbuf_writepoint = outbuf + writer_offset;
}


void file_reader::shrink_outfile(size_t how_much)
{
  //TODO
}


file_loader& file_loader::operator=(const file_loader& fr)
{
  //TODO: may add boolean for equals operator to determine if is buffered_or_not
  terminate();
  //1. copy data
  //2. backpoint and frontpoint
  //3. inbuf as ptr or mmap
  //4. outbuf as ptr or mmap
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
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos)
{
  file_loader = {nullptr};
  constructionLogic();
  copyDelimitersFromString(delimitadoresPalabra);
}


Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.minuscSinAcentos), file_loader()
{
  constructionLogic();
  copyDelimiters(tk.delimitadoresPalabra);
}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false), file_loader()
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
  loader = tk.loader;
}


void Tokenizador::Tokenizar(const string& str, list<string>& tokens) const
{
  //TODO: token pipeline: extract -> normalize -> state machine -> put (+endl ? reset state machine)

  //TODO: substitute string buffer
  string buffer(str);
  tokens.clear();
  const char *strbegin = str.c_str(), const char *const strend = strbegin + str.length();
  while(strbegin < strend)
  {
    // ALL THIS inside a superloop because of cyclic reads
    // 1. Copy data
    // 2. Normalize data
    // 3. State machine
    // 4. Push back string into list
  }

  //TODO: solve this shit
  if(pasarAminuscSinAcentos)
    normalizeStream(buffer);
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


extern inline char Tokenizador::normalizeChar(const char c)
{
  int16_t curChar = static_cast<int16_t>(c);
  if(curChar >= CAPITAL_START_POINT && curChar <= CAPITAL_END_POINT)
    curChar += Tokenizador::TOLOWER_OFFSET;
  else if(curChar - ACCENT_START_POINT >= 0)
    curChar += Tokenizador::accentRemovalOffsetVec[curChar - ACCENT_START_POINT];

  return static_cast<char>(curChar);
}


const void* Tokenizador::tmpStore(const void* const chunk, const size_t size)
{
  const char* chunk_end = static_cast<const char*>(chunk) + size;
  const int64_t* readpoint = static_cast<const int64_t*>(chunk);
  int64_t* writepoint = reinterpret_cast<int64_t*>(tmpData);
  const char* remain_readpoint = chunk_end - (chunk_end & 0b111);
  char* remain_writepoint = tmpDataEnd - (tmpDataEnd & 0b111);

  while(writepoint < reinterpret_cast<int64_t*>(remain_writepoint) && readpoint < reinterpret_cast<int64_t*>(remain_readpoint))
  {
    *writepoint = *readpoint;
    ++writepoint;
    ++readpoint;
  }

  while(remain_writepoint < tmpDataEnd && remain_readpoint < chunk_end)
  {
    *remain_writepoint = *remain_readpoint;
    ++remain_writepoint;
    ++remain_readpoint;
  }
  return (size > (tmpDataEnd - tmpData) ? reinterpret_cast<void*>(remain_readpoint) : nullptr);
}
