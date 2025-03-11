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

  if(!output_filename) return true;
  fd = open(output_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if(fd < 0)
  {
    munmap(const_cast<char*>(inbuf), in_fileinfo.st_size);
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
  munmap(const_cast<char*>(inbuf), inbuf_size);

  if(outbuf) {
    munmap(outbuf, outbuf_size);
    if(remaining_writepoint && (outbuf + outbuf_size) - remaining_writepoint)  // Adjust filesize
    {
      int fd = open(outbuf_filename, O_WRONLY);
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


extern inline void file_loader::null_writepoints()
{
  outbuf = nullptr;
  outbuf_size = 0;
  outbuf_filename = nullptr;
}


void file_loader::grow_outfile(size_t how_much, const char** wr_beg, const char** wr_cur, const char** wr_end)
{
  munmap(outbuf, outbuf_size);
  how_much += outbuf_size;

  int fd = open(outbuf_filename, O_WRONLY);
  ftruncate(fd, how_much);
  outbuf = (char*)mmap(nullptr, how_much, PROT_WRITE, MAP_SHARED, fd, 0);
  outbuf_size = how_much;
  close(fd);
  
  size_t back_diff = *wr_cur - *wr_beg;
  *wr_beg = outbuf;
  *wr_cur = outbuf + back_diff;
  *wr_end = outbuf + outbuf_size;
}


void memory_pool::write(const void *const chunk, const size_t size, const off_t wrstart = 0)
{
  memory_pool::mv(chunk, this->data + wrstart, size);
}


void memory_pool::read(void *const dst, const size_t size, const off_t rdstart = 0)
{
  memory_pool::mv(this->data + rdstart, dst, size);
}


void memory_pool::start_put_mode()
{
  putbuf = data;
  putpoint = putbuf;
  put_end = putbuf + MEM_POOL_SIZE;
  putbuf_size = MEM_POOL_SIZE;
}


void memory_pool::put(const char c)
{
  if(putbuf < put_end)
  {
    *putpoint = c;
    ++putpoint;
  }
  else if(putbuf != data)
  {
    putbuf_size += MEM_POOL_SIZE;
    char* newbuf = new char[putbuf_size];
    mem_pool::mv(putbuf, newbuf, putbuf_size - MEM_POOL_SIZE);
    putpoint = newbuf + (putpoint - putbuf);
    delete[] putbuf;
    putbuf = newbuf;
    put_end = newbuf + putbuf_size;
    put(c);
  }
  else
  {
    putbuf_size += MEM_POOL_SIZE;
    putbuf = new char[putbuf_size];
    mem_pool::mv(data, putbuf, MEM_POOL_SIZE);
    putpoint = putbuf + (putpoint - data);
    put_end = putbuf + putbuf_size;
    put(c);
  }
}


const char* memory_pool::get(const char end = '\0')
{
  put(end);
  return putbuf;
}


void memory_pool::end_put_mode()
{
  if(putbuf != data && putbuf)
    delete[] putbuf;
  putbuf = nullptr;
}


void memory_pool::mv(const void *const src, void *const dst, const size_t size)
{
  const int64_t* readpoint = reinterpret_cast<const int64_t*>(src);
  int64_t* writepoint = reinterpret_cast<int64_t*>(dst);
  const char *const readpoint_end = reinterpret_cast<const char *const>(src) + size;
  const char *const writepoint_end = reinterpret_cast<const char *const>(dst) + size;
  const char* remain_readpoint = readpoint_end - (reinterpret_cast<const int64_t>(readpoint_end) & 0b111);
  char* remain_writepoint = const_cast<char*>(writepoint_end) - (reinterpret_cast<const int64_t>(writepoint_end) & 0b111);

  while(writepoint < reinterpret_cast<int64_t*>(remain_writepoint) && readpoint < reinterpret_cast<const int64_t*>(remain_readpoint))
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
    if(tk.checkDelimiter(static_cast<char>(i)))
      cout << reinterpret_cast<char>(i);

  cout << " TRATA DE CASOS ESPECIALES: " << tk.casosEspeciales 
    << " PASAR A MINUSCULAS Y SIN ACENTOS: " << tk.pasarAminuscSinAcentos
    << flush;
}


Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool casosEspeciales, const bool minuscSinAcentos) :
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos)
{
  loader = {0};
  constructionLogic();
  copyDelimitersFromString(delimitadoresPalabra);
}


Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.pasarAminuscSinAcentos)
{
  constructionLogic();
  copyDelimiters(tk.delimitadoresPalabra);
  loader = {0};
}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false)
{
  static const char* delimDefaults = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  const char* i = delimDefaults;
  constructionLogic();
  while(*i != '\0')
  {
    setDelimiter(*i, true);
    ++i;
  }
  loader = {0};
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
  rdend = rdbegin + str.length();
  wrbegin = mem_pool.data;
  wr_current = const_cast<char*>(wrbegin);
  wrend = wrbegin + MEM_POOL_SIZE;

  const char* tk;
  while(rd_current < rdend)
  {
    wr_current = mem_pool.data;
    tk = extractToken();
    tokens.push_back(tk);
  }
}


bool Tokenizador::Tokenizar(const string& i, const string& f)
{
  if(!loader.begin(i.c_str(), f.c_str()))
  {
    cerr << "ERROR: No existe el archivo: " << i << endl;
    return false;
  }
  rdbegin = loader.inbuf;
  rd_current = rdbegin;
  rdend = rdbegin + loader.inbuf_size;
  wrbegin = loader.outbuf;
  wr_current = const_cast<char*>(wrbegin);
  wrend = wrbegin + loader.outbuf_size;

  const char* tk;
  while(rd_current < rdend)
  {
    extractToken('\n');
    ensureOutfileHasEnoughMem();
  }

  loader.terminate(wr_current);
  return true;
} 


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
  struct file_loader lista_ficheros;
  if(!lista_ficheros.begin(i)) return false;
  string cur_file;
  while(getline(lista_ficheros.inbuf, cur_file))
  {
    Tokenizar(cur_file, cur_file + ".tk");
  }

  lista_ficheros.terminate();
  return true;
} 


bool Tokenizador::TokenizarDirectorio(const string& dirAIndexar)
{
  struct stat dir;
  int err = stat(dirAIndexar.c_str(), &dir);
  if(err == -1 || S_ISDIR(dir.st_mode))
    return false;
  else {
    string cmd = "find " + dirAIndexar + " -follow | sort > .lista_fich";
    system(cmd.c_str());
  }
  return TokenizarListaFicheros(".lista_fich");
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


const char* Tokenizador::extractToken(char last = '\0')
{
  /*
  if(casosEspeciales)
  {
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
  */
  //TODO: tmp till special cases
  while(checkDelimiter(*rd_current)) ++rd_current;
  while(!checkDelimiter(*rd_current))
  {
    *wr_current = (pasarAminuscSinAcentos ? normalizeChar(*rd_current) : *rd_current);
    ++rd_current;
    ++wr_current;
  }
  *wr_current = last;
  ++rd_current;
  ++wr_current;
  return wrbegin;
}


extern inline void Tokenizador::ensureOutfileHasEnoughMem()
{
    if(rdend - rd_current > wrend - wr_current)
    {
      size_t difference = (rdend - rd_current) - (wrend - wr_current);
      loader.grow_outfile(difference, &wrbwgin, &wr_current, &wrend);
      rdend += difference;
    }
}


extern inline void Tokenizador::constructionLogic()
{
  resetDelimiters();
  setDelimiter(0x20 /*empty space*/, true);
  setDelimiter('\0', true);
  setDelimiter('\n', true);
}
