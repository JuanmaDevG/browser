#include <tokenizador.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

//TODO: uninclude include this when possible
#include<fstream>


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


void file_loader::grow_outfile(size_t how_much, io_context& bound_ioc)
{
  munmap(outbuf, outbuf_size);
  how_much += outbuf_size;

  int fd = open(outbuf_filename, O_WRONLY);
  ftruncate(fd, how_much);
  outbuf = (char*)mmap(nullptr, how_much, PROT_WRITE, MAP_SHARED, fd, 0);
  outbuf_size = how_much;
  close(fd);
  
  size_t back_diff = bound_ioc.wr_current - bound_ioc.wrbegin;
  bound_ioc.wrbegin = outbuf;
  bound_ioc.wr_current = outbuf + back_diff;
  bound_ioc.wrend = outbuf + outbuf_size;
}


void memory_pool::write(const void *const chunk, const size_t size, const off_t wrstart = 0)
{
  memory_pool::mv(chunk, this->data + wrstart, size);
}


void memory_pool::read(void *const dst, const size_t size, const off_t rdstart = 0)
{
  memory_pool::mv(this->data + rdstart, dst, size);
}


extern inline size_t memory_pool::size()
{
  return data_end - data;
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


void io_context::swap(io_context& ioc)
{
  std::swap(rdbegin, ioc.rdbegin);
  std::swap(rd_current, ioc.rd_current);
  std::swap(rdend, ioc.rdend);
  std::swap(wrbegin, ioc.wrbegin);
  std::swap(wr_current, ioc.wr_current);
  std::swap(wrend, ioc.wrend);
}


extern inline bool iso_8859_1_bitvec::check(const uint8_t delim_idx) const
{
  return static_cast<bool>((this->data[delim_idx >> 3] >> (delim_idx & 0b111)) & 1);
}


void iso_8859_1_bitvec::set(const uint8_t delim_idx, const bool val)
{
  if(val)
    this->data[delim_idx >> 3] = this->data[delim_idx >> 3] | (1 << (static_cast<uint8_t>(delim_idx) & 0b111));
  else
    this->data[delim_idx >> 3] = this->data[delim_idx >> 3] & (0xff - (1 << (static_cast<uint8_t>(delim_idx) & 0b111)));
}


void iso_8859_1_bitvec::reset()
{
  int64_t* d = reinterpret_cast<int64_t*>(data);
  const int64_t* end = reinterpret_cast<int64_t*>(data) + AMD64_REGISTER_VEC_SIZE;
  while(d < end)
  {
    *d = 0;
    ++d;
  }
}


void iso_8859_1_bitvec::copy_from(const iso_8859_1_bitvec& delim)
{
  memory_pool::mv(delim.data, this->data, DELIMITER_BIT_VEC_SIZE);
}


void iso_8859_1_bitvec::copy_from(const string& delim_str)
{
  for(auto reader = delim_str.cbegin(); reader != delim_str.cend(); reader++)
    this->set(*reader, true);
}


void iso_8859_1_bitvec::copy_to(iso_8859_1_bitvec& dst) const
{
  memory_pool::mv(this->data, dst.data, DELIMITER_BIT_VEC_SIZE);
}


ostream& operator<<(ostream& os, const Tokenizador& tk)
{
  cout << "DELIMITADORES: ";
  for(uint8_t i=0;; ++i)
  {
    if(tk.delimiters.check(i))
    {
      cout << (char)i;
      /*
      switch((char)i)
      {
        case '\n':
          cout << "\\n";
          break;
        case '\t':
          cout << "\\t";
          break;
        case '\\':
          cout << "\\";
          break;
        default:
          cout << (char)i;
      }
      */
    }

    if(i == ISO_8859_SIZE -1)
      break;
  }

  cout << " TRATA DE CASOS ESPECIALES: " << tk.casosEspeciales 
    << " PASAR A MINUSCULAS Y SIN ACENTOS: " << tk.pasarAminuscSinAcentos
    << flush;

  return os;
}


Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool casosEspeciales, const bool minuscSinAcentos) :
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos)
{
  loader = {0};
  constructionLogic();
  delimiters.copy_from(delimitadoresPalabra);
}


Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.pasarAminuscSinAcentos)
{
  constructionLogic();
  delimiters.copy_from(tk.delimiters);
  loader = {0};
}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false)
{
  static const char* delimDefaults = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  const char* i = delimDefaults;
  constructionLogic();
  while(*i != '\0')
  {
    delimiters.set(*i, true);
    ++i;
  }
  loader = {0};
}


Tokenizador::~Tokenizador()
{
  delimiters.reset();
  loader.terminate();
}


Tokenizador& Tokenizador::operator=(const Tokenizador& tk)
{
  casosEspeciales = tk.casosEspeciales;
  pasarAminuscSinAcentos = tk.pasarAminuscSinAcentos;
  delimiters.copy_from(tk.delimiters);
  loader.terminate();
  return *this;
}


void Tokenizador::Tokenizar(const string& str, list<string>& tokens)
{
  tokens.clear();
  ioctx.rdbegin = str.c_str();
  ioctx.rd_current = ioctx.rdbegin;
  ioctx.rdend = ioctx.rdbegin + str.length();
  setMemPool();

  const char* tk;
  while(ioctx.rd_current < ioctx.rdend)
  {
    ioctx.wr_current = ioctx.wrbegin;
    tk = (this->*extractToken)('\0');
    tokens.push_back(tk);
  }
  unsetMemPool();
}


bool Tokenizador::Tokenizar(const string& i, const string& f)
{
  if(!loader.begin(i.c_str(), f.c_str()))
  {
    cerr << "ERROR: No existe el archivo: " << i << endl;
    return false;
  }
  //TODO: make func to bind ioctx to file_loader or memory_pool
  ioctx.rdbegin = loader.inbuf;
  ioctx.rd_current = ioctx.rdbegin;
  ioctx.rdend = ioctx.rdbegin + loader.inbuf_size;
  ioctx.wrbegin = loader.outbuf;
  ioctx.wr_current = ioctx.wrbegin;
  ioctx.wrend = ioctx.wrbegin + loader.outbuf_size;

  const char* tk;
  while(ioctx.rd_current < ioctx.rdend)
  {
    (this->*extractToken)('\n');
    ensureOutfileHasEnoughMem();
  }

  loader.terminate(ioctx.wr_current);
  return true;
} 


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
  //TODO: optimize (when possible)
  ifstream list_file(i);
  if(!list_file.is_open()) return false;
  string cur_file;
  while(getline(list_file, cur_file))
  {
    Tokenizar(cur_file, cur_file + ".tk");
  }

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
  delimiters.reset();
  delimiters.copy_from(nuevoDelimiters);
} 


void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{
  delimiters.copy_from(nuevoDelimiters);
}


string Tokenizador::DelimitadoresPalabra() const
{
  string result;
  result.reserve(ISO_8859_SIZE);
  for(uint8_t i=0; i < ISO_8859_SIZE; ++i)
  {
    if(delimiters.check(static_cast<char>(i)))
      result.push_back(static_cast<char>(i));
  }

  return result;
} 


void Tokenizador::CasosEspeciales(const bool nuevoCasosEspeciales)
{
  casosEspeciales = nuevoCasosEspeciales;
  extractToken = (casosEspeciales ? &Tokenizador::extractSpecialCaseToken : &Tokenizador::extractCommonCaseToken);
}


bool Tokenizador::CasosEspeciales() const
{
  return casosEspeciales;
}


void Tokenizador::PasarAminuscSinAcentos(const bool nuevoPasarAminuscSinAcentos)
{
  pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
  normalizeChar = (pasarAminuscSinAcentos ? &Tokenizador::minWithoutAccent : &Tokenizador::rawCharReturn);
}


bool Tokenizador::PasarAminuscSinAcentos() const
{
  return pasarAminuscSinAcentos;
}


//TODO: change this to loader.makeiowr_safe(ioctx) or ioctx.ensure_outfile_mem(file_loader)
//TODO: consider adding an object reference to some of the structs
extern inline void Tokenizador::ensureOutfileHasEnoughMem()
{
  if(ioctx.rdend - ioctx.rd_current > ioctx.wrend - ioctx.wr_current)
  {
    size_t difference = (ioctx.rdend - ioctx.rd_current) - (ioctx.wrend - ioctx.wr_current);
    loader.grow_outfile(difference, ioctx);
    ioctx.rdend += difference;
  }
}


void Tokenizador::setMemPool()
{
  ioctx.wrbegin = mem_pool.data;
  ioctx.wr_current = ioctx.wrbegin;
  ioctx.wrend = mem_pool.data_end;
  writeChar = &Tokenizador::safeMemPoolWrite;
}


void Tokenizador::unsetMemPool()
{
  if(ioctx.wrbegin && ioctx.wrbegin != loader.outbuf && ioctx.wrbegin != mem_pool.data)
  {
    delete[] ioctx.wrbegin;
    ioctx.wrbegin = nullptr;
    writeChar = &Tokenizador::rawWrite;
  }
}


void Tokenizador::putTerminatingChar(const char c)
{
  if(ioctx.rd_current >= ioctx.rdend)
    ioctx.rd_current -= (ioctx.rd_current - ioctx.rdend) +1;
  (this->*writeChar)(); //To ensure buffer safety
  --ioctx.wr_current;
  *ioctx.wr_current = c;
  ++ioctx.wr_current;
}


void Tokenizador::skipDelimiters(const bool leaveLastOne)
{
  if(!delimiters.check(*ioctx.rd_current)) return;
  while(ioctx.rd_current < ioctx.rdend && delimiters.check(*ioctx.rd_current))
    ++ioctx.rd_current;

  if(leaveLastOne)
    --ioctx.rd_current;
}


bool Tokenizador::isNumeric(const char c) const
{
  return static_cast<uint8_t>(c) >= Tokenizador::NUMERIC_START_POINT && static_cast<uint8_t>(c) <= Tokenizador::NUMERIC_END_POINT;
}


const char* Tokenizador::extractCommonCaseToken(const char last)
{
  const char *const ini_wrptr = ioctx.wr_current;
  skipDelimiters(false);
  while(ioctx.rd_current < ioctx.rdend && !delimiters.check(*ioctx.rd_current))
  {
    (this->*writeChar)();
  }

  putTerminatingChar(last);
  return ini_wrptr;
}


const char* Tokenizador::extractSpecialCaseToken(const char last)
{
  skipDelimiters(true);
  const char *const ini_wrptr = ioctx.wr_current;
  const char* tk_end;

  tk_end = decimalTill();
  if(tk_end)
  {
    if(*ioctx.rd_current == ',' || *ioctx.rd_current == '.')
    {
      putTerminatingChar('0');
      --ioctx.rd_current; // Advances the rdptr so make it stay
    }
  }
  else
  {
    ++ioctx.rd_current; // Pass delimiter
    tk_end = multiwordTill();
    if(!tk_end)
      tk_end = urlTill();
    if(!tk_end)
      tk_end = emailTill();
    if(!tk_end)
      tk_end = acronymTill();
  }

  if(!tk_end)
    return extractCommonCaseToken(last);

  while(ioctx.rd_current < tk_end)
    (this->*writeChar)();
  putTerminatingChar(last);

  return ini_wrptr;
}


void Tokenizador::rawWrite()
{
  *ioctx.wr_current = (this->*normalizeChar)(*ioctx.rd_current);
  ++ioctx.wr_current;
  ++ioctx.rd_current;
}


void Tokenizador::safeMemPoolWrite()
{
  if(ioctx.wr_current < ioctx.wrend)
  {
    rawWrite();
  }
  else if(ioctx.wr_current != mem_pool.data)
  {
    const size_t old_sz = (ioctx.wrend - ioctx.wrbegin);
    const size_t new_sz = old_sz + MEM_POOL_SIZE;
    char* newbuf = new char[new_sz];
    memory_pool::mv(ioctx.wrbegin, newbuf, old_sz);
    ioctx.wr_current = newbuf + old_sz;
    delete[] ioctx.wrbegin;
    ioctx.wrbegin = newbuf;
    ioctx.wrend = newbuf + new_sz;
    rawWrite();
  }
  else
  {
    const size_t new_sz = MEM_POOL_SIZE + MEM_POOL_SIZE;
    ioctx.wrbegin = new char[new_sz];
    memory_pool::mv(mem_pool.data, ioctx.wrbegin, MEM_POOL_SIZE);
    ioctx.wr_current = ioctx.wrbegin + MEM_POOL_SIZE;
    ioctx.wrend = ioctx.wrbegin + new_sz;
    rawWrite();
  }
}


extern inline char Tokenizador::rawCharReturn(const char c)
{
  return c;
}


char Tokenizador::minWithoutAccent(const char c)
{
  int16_t curChar = static_cast<int16_t>(c);
  if(curChar >= CAPITAL_START_POINT && curChar <= CAPITAL_END_POINT)
    curChar += Tokenizador::TOLOWER_OFFSET;
  else if(curChar - ACCENT_START_POINT >= 0)
    curChar += Tokenizador::accentRemovalOffsetVec[curChar - ACCENT_START_POINT];

  return static_cast<char>(curChar);
}


//TODO: ask the teacher if an accented or capital leter can be delimiter (¿getChar() everywhere?)
const char* Tokenizador::multiwordTill()
{
  const char* reader = ioctx.rd_current;
  if(!delimiters.check('-') || delimiters.check(*reader))
    return nullptr;

  while(reader < ioctx.rdend && !delimiters.check(*reader))
    ++reader;
  if(reader == ioctx.rdend || *reader != '-' || reader +1 == ioctx.rdend || delimiters.check(*(reader +1)))
    return nullptr;

  reader += 2;
  while(reader < ioctx.rdend)
  {
    // hyphen allowed
    while(reader < ioctx.rdend && !delimiters.check(*reader))
      ++reader;

    // no hyphen means end delimiter
    if(reader == ioctx.rdend || *reader != '-' || reader +1 == ioctx.rdend || delimiters.check(*(reader +1)))
      return reader;
    ++reader;
  }
  
  return reader;
}


//TODO: make url_delim constexpr
const char* Tokenizador::urlTill()
{
  const char* reader = ioctx.rd_current;
  iso_8859_1_bitvec url_delim;
  url_delim.reset();
  url_delim.copy_from("_:/.?&-=#@");

  if(
    reader +4 >= ioctx.rdend 
    || (!url_delim.check(*(reader +4)) && delimiters.check(*(reader +4)))
    || !(
      ((this->*normalizeChar)(*reader) == 'f' 
       && (this->*normalizeChar)(*(reader +1)) == 't' 
       && (this->*normalizeChar)(*(reader +2)) == 'p' 
       && *(reader +3) == ':')
      || ((this->*normalizeChar)(*reader) == 'h' 
        && (this->*normalizeChar)(*(reader +1)) == 't' 
        && (this->*normalizeChar)(*(reader +2)) == 't' 
        && (this->*normalizeChar)(*(reader +3)) == 'p')))
  {
    return nullptr;
  }
  reader += 4;

  if((this->*normalizeChar)(*reader) == 's' && *(reader +1) == ':')
    reader += 2;
  if(!(url_delim.check(*reader) || !delimiters.check(*reader)))
    return nullptr;
  ++reader;

  while(reader < ioctx.rdend)
  {
    if(delimiters.check(*reader) && !url_delim.check(*reader))
      return reader;
    ++reader;
  }

  return reader;
}


const char* Tokenizador::emailTill()
{
  if(!delimiters.check('@') || *ioctx.rd_current == '@')
    return nullptr;

  const char* reader = ioctx.rd_current;
  bool found_at_sign = false;

  while(reader < ioctx.rdend && !found_at_sign)
  {
    if(*reader == '@')
    {
      found_at_sign = true;
      ++reader;
      continue;
    }
    if(delimiters.check(*reader))
      return nullptr;

    ++reader;
  }
  if(!found_at_sign)
    return nullptr;

  iso_8859_1_bitvec sufix_delimiters; //TODO: make constexpr when possible
  sufix_delimiters.reset();
  sufix_delimiters.copy_from(".-_");
  while(reader < ioctx.rdend)
  {
    if(*reader == '@')
      return nullptr; // Not allowed second @
    if(
        sufix_delimiters.check(*reader) 
        && (delimiters.check(*(reader -1)) || reader +1 == ioctx.rdend || delimiters.check(*(reader +1))))
    {
      return nullptr;
    }
    else if(delimiters.check(*reader))
      return reader;

    ++reader;
  }
  return reader;
}


const char* Tokenizador::acronymTill()
{
  if(!delimiters.check('.'))
    return nullptr;

  const char* reader = ioctx.rd_current;
  while(reader < ioctx.rdend)
  {
    if(
        *reader == '.' && (reader +1 == ioctx.rdend || delimiters.check(*(reader +1)))
        || *reader != '.' && delimiters.check(*reader))
    {
      return reader;
    }
    ++reader;
  }
  return reader;
}

const char* Tokenizador::decimalTill()
{
  if(!(delimiters.check('.') && delimiters.check(',')))
    return nullptr;
  const char* reader = ioctx.rd_current;
  const char *dot = nullptr, *comma = nullptr;

  if(*reader == '.')
    dot = reader;
  else if(*reader == ',')
    comma = reader;
  else
    ++ioctx.rd_current;
  ++reader;

  if(!isNumeric(*reader))
    return nullptr;
  else
    ++reader;

  // Guaranteed at least one numeric char
  while(reader < ioctx.rdend)
  {
    if(*reader == '.')
    {
      if(dot) return nullptr;
      dot = reader;
    }
    if(*reader == ',')
    {
      if(comma) return nullptr;
      comma = reader;
      if(dot && comma > dot) return comma;
    }
    else if(delimiters.check(*reader))
    {
      if(*(reader -1) == '.' || *(reader -1) == ',') // Cut the point if useless
        --reader;
      if(dot && comma && comma > dot) return comma;
      return reader;
    }
    else if(!isNumeric(*reader))
    {
      if(!(dot || comma)) return nullptr;
      return reader;
    }
    ++reader;
  }
  return reader;
}


extern inline void Tokenizador::constructionLogic()
{
  delimiters.reset();
  delimiters.set(0x20 /*empty space*/, true);
  delimiters.set('\0', true);
  delimiters.set('\n', true);
  ioctx.rdbegin = nullptr;
  ioctx.wrbegin = nullptr;
  writeChar = &Tokenizador::rawWrite;
  
  if(casosEspeciales)
    extractToken = &Tokenizador::extractSpecialCaseToken;
  else
    extractToken = &Tokenizador::extractCommonCaseToken;

  if(pasarAminuscSinAcentos)
    normalizeChar = &Tokenizador::minWithoutAccent;
  else
    normalizeChar = &Tokenizador::rawCharReturn;
}
