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
  inbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  inbuf_size = in_fileinfo.st_size;
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
  outbuf_size = in_fileinfo.st_size;
  outbuf_filename = output_filename;

  return true;
}


void file_loader::terminate(char* const remaining_writepoint = nullptr)
{
  if(!inbuf) return;
  munmap(inbuf, inbuf_size);

  if(outbuf) {
    munmap(outbuf, outbuf_size);
    if(remaining_wirtepoint && (outbuf + outbuf_size) - remaining_writepoint)  // Adjust filesize
    {
      int fd = open(output_filename, O_WRONLY);
      ftruncate(fd, outbuf_size - ((outbuf + outbuf_size) - remaining_writepoint));
      close(fd);
    }
  }

  null_readpoints();
  null_writepoints();
}


extern inline void file_loader::null_readpoints()
{
  inbuf = nullptr;
  inbuf_size = 0;
}


extern inline void file_reader::null_writepoints()
{
  outbuf = nullptr;
  outbuf_size = 0;
  outbuf_filename = nullptr;
}


void file_reader::grow_outfile(size_t how_much)
{
  munmap(outbuf, outbuf_size);
  how_much += outbuf_size;

  int fd = open(outbuf_filename, O_WRONLY);
  ftruncate(fd, how_much);
  outbuf = mmap(nullptr, how_much, PROT_WRITE, MAP_SHARED, fd, 0);
  outbuf_size = how_much;
  close(fd);
}


const void* memory_pool::write(const void *const chunk, const size_t size, const off_t wrstart = 0)
{
  memory_pool::mv(chunk, this->data + wrstart, size);
}


bool memory_pool::read(void *const dst, const size_t size, const off_t rdstart = 0)
{
  memory_pool::mv(this->data + rdstart, dst, size);
}


extern inline void memory_pool::mv(const void *const src, void *const dst, const size_t size)
{
  const int64_t* readpoint = reinterpret_cast<const int64_t*>(src);
  int64_t* writepoint = reinterpret_cast<int64_t*>(dst);
  const char *const redpoint_end = reinterpret_cast<const char *const>(src) + size;
  const char *const writepoint_end = reinterpret_cast<const char *const>(dst) + size;
  const char* remain_readpoint = readpoint_end - (readpoint_end & 0b111);
  char* remain_writepoint = const_cast<char*>(writepoint_end) - (const_cast<char*>(writepoint_end) & 0b111);

  while(writepoint < reinterpret_cast<int64_t*>(remain_writepoint) && readpoint < reinterpret_cast<int64_t*>(remain_readpoint))
  {
    *writepoint = *readpoint;
    ++writepoint;
    ++readpoint;
  }

  while(remain_writepoint < writepoint_end && remain_readpoint < readpoint_end)
  {
    *remain_writepoint = *remain_readpoint;
    ++remain_writepoint;
    ++remain_readpoint;
  }
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
  const char* i = delimDefaults;
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
  loader.terminate();
}


Tokenizador::Tokenizador& operator=(const Tokenizador& tk)
{
  casosEspeciales = tk.casosEspeciales;
  pasarAminuscSinAcentos = tk.pasarAminuscSinAcentos;
  copyDelimiters(tk.delimitadoresPalabra);
  loader.terminate();
}


void Tokenizador::Tokenizar(const string& str, list<string>& tokens)
{
  tokens.clear();
  rdbegin = str.c_str();
  rd_current = rdbegin;
  rdend = backpoint + str.length();
  wrbegin = mem_pool.data;
  wr_current = wrbegin;
  wrend = wrbegin + MEM_POOL_SIZE;

  while(rd_current < rdend)
  {
    //TODO: call extractToken
    if(casosEspeciales)
    {
    }
    else
    {
    }
    ++readpoint;
  }
}


bool Tokenizador::Tokenizar(const string& i, const string& f)
{
  //TODO: open file -> mmap -> copy to buffer -> normalize -> tokenize -> if not enough info then do partial load
} 


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
} 


bool Tokenizador::TokenizarDirectorio(const string& i)
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


extern inline uint8_t& _getDelimiterMemChunk(const char delim)
{
  return this->delimitadoresPalabra[reinterpret_cast<uint8_t>(x) >> 3];
}


extern inline bool Tokenizador::checkDelimiter(const char delim_idx) const
{
  return (_getDelimiterMemChunk(delim_idx) >> (delim_idx & 0b111)) & 1;
}


extern inline void Tokenizador::setDelimiter(const char delim_idx, const bool val)
{
  if(val)
    _getDelimiterMemChunk(delim_idx) = _getDelimiterMemChunk(delim_idx) | (1 << (reinterpret_cast<uint8_t>(delim_idx) & 0b111));
  else
    _getDelimiterMemChunk(delim_idx) = _getDelimiterMemChunk(delim_idx) & (0xff - (1 << (reinterpret_cast<uint8_t>(delim_idx) & 0b111)));
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


extern inline char Tokenizador::normalizeChar(const char c)
{
  int16_t curChar = static_cast<int16_t>(c);
  if(curChar >= CAPITAL_START_POINT && curChar <= CAPITAL_END_POINT)
    curChar += Tokenizador::TOLOWER_OFFSET;
  else if(curChar - ACCENT_START_POINT >= 0)
    curChar += Tokenizador::accentRemovalOffsetVec[curChar - ACCENT_START_POINT];

  return static_cast<char>(curChar);
}


const char* extractToken()
{
  if(casosEspeciales)
  {
    //TODO: activar casos especiales
  }
  else
  {
    while(checkDelimiter(*rd_current)) ++rd_current;
    while(!checkDelimiter(*rd_current))
    {
      *wr_current = (pasarAminuscSinAcentos ? normalizeChar(*rd_current) : *rd_current);
      ++rd_current;
      ++wr_current;
    }
    *wr_current = '\0';
    ++rd_current;
    ++wr_current;
  }
  return wrbegin;
}


extern inline void Tokenizador::constructionLogic()
{
  resetDelimiters();
  if(casosEspeciales)
    setDelimiter(0x20 /*empty space*/, true);
}
