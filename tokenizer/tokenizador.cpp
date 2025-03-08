#include <tokenizador.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


bool file_loader::begin(const char* filename, const char* output_filename = nullptr)
{
  int fd = open(filename, O_RDONLY);
  struct stat in_fileinfo;
  if(fd < 0)
    return false;

  fstat(fd, &in_fileinfo);
  inbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
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
    inbuf = nullptr;
    inbuf_end = nullptr;
    backpoint = nullptr;
    frontpoint = nullptr;
    inbuf_filename = nullptr;
    return false;
  }
  ftruncate(fd, in_fileinfo.st_size); // As minimum, the input file size is enough
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
  inbuf = nullptr;
  inbuf_end = nullptr;
  backpoint = nullptr;
  frontpoint = nullptr;
  inbuf_filename = nullptr;

  if(outbuf) {
    munmap(outbuf, static_cast<size_t>(outbuf_end - outbuf));
    if(outbuf_end - outbuf_writepoint)  // Adjusting the filesize to the bytes written
    {
      int fd = open(output_filename, O_WRONLY);
      ftruncate(fd, (outbuf_end - outbuf) - (outbuf_end - outbuf_writepoint));
    }

    outbuf = nullptr;
    outbuf_end = nullptr;
    outbuf_writepoint = nullptr;
    outbuf_filename = nullptr;
  }
}


void file_loader::write(const char chunk_end)
{
  if(frontpoint - backpoint +1 > outbuf_end - outbuf_writepoint)
  {
    off_t writer_offset = outbuf_writepoint - outbuf;
    size_t filesize = static_cast(outbuf_end - outbuf);
    munmap(outbuf, filesize);
    filesize += 256;

    int fd = open(outbuf_filename, O_WRONLY);
    ftruncate(fd, filesize);
    outbuf = mmap(nullptr, filesize, PROT_WRITE, MAP_SHARED, fd, 0);
    outbuf_end = outbuf + filesize;
    outbuf_writepoint = outbuf + writer_offset;
  }
  const int64_t* big_readpoint = reinterpret_cast<const int64_t*>(backpoint);
  int64_t* big_writepoint = reinterpret_cast<int64_t*>(outbuf_writepoint);
  backpoint += (frontpoint - backpoint) - ((frontpoint - backpoint) & 0b111); // Backpoint placed at remaining bytes
  
  while(big_readpoint < backpoint)
  {
    *big_writepoint = *big_readpoint;
    ++big_readpoint;
    ++big_writepoint;
  }
  outbuf_writepoint = reinterpret_cast<char*>(big_writepoint);

  if(backpoint < frontpoint)
  {
    //TODO: write remaining bytes [backpoint -> frontpoint] using outbuf_writepoint
  }
  *outbuf_writepoint = chunk_end;
  ++outbuf_writepoint;
}


file_loader& file_loader::operator=(const file_loader& fr)
{
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


//TODO: probably delete this function in favour of file_loader functions
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
