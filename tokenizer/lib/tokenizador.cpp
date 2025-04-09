#include "tokenizador.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>


file_loader::file_loader()
{
  null_readpoints();
  null_writepoints();
}


bool file_loader::begin(const char* filename, const char* output_filename = nullptr)
{
  inbuf_fd = open(filename, O_RDONLY);
  struct stat in_fileinfo;
  if(inbuf_fd < 0)
    return false;

  fstat(inbuf_fd, &in_fileinfo);
  inbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, inbuf_fd, 0);
  inbuf_size = in_fileinfo.st_size;
  readpoint = inbuf;
  frontpoint = inbuf;
  inbuf_end = inbuf + inbuf_size;

  if(!output_filename) return true;
  outbuf_fd = open(output_filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if(outbuf_fd < 0)
  {
    munmap(const_cast<char*>(inbuf), in_fileinfo.st_size);
    null_readpoints();
    return false;
  }
  ftruncate(outbuf_fd, in_fileinfo.st_size);
  outbuf = (char*)mmap(nullptr, in_fileinfo.st_size, PROT_WRITE, MAP_SHARED, outbuf_fd, 0);
  if(outbuf == MAP_FAILED)
  {
    close(inbuf_fd);
    close(outbuf_fd);
    munmap(const_cast<char*>(inbuf), inbuf_size);
    null_readpoints();
    null_writepoints();
    return false;
  }
  outbuf_size = in_fileinfo.st_size;
  outbuf_end = outbuf + outbuf_size;
  writepoint = outbuf;

  return true;
}


void file_loader::terminate()
{
  if(!inbuf) return;
  munmap(const_cast<char*>(inbuf), inbuf_size);
  close(inbuf_fd);

  if(outbuf) {
    munmap(outbuf, outbuf_size);
    if(writepoint < outbuf_end)  // Adjust filesize
    {
      ftruncate(outbuf_fd, outbuf_size - (outbuf_end - writepoint));
    }
    close(outbuf_fd);
  }

  null_readpoints();
  null_writepoints();
}


extern inline void file_loader::null_readpoints()
{
  inbuf = nullptr;
  inbuf_end = nullptr;
  inbuf_size = 0;
  readpoint = nullptr;
  frontpoint = nullptr;
  inbuf_fd = 0;
}


extern inline void file_loader::null_writepoints()
{
  outbuf = nullptr;
  outbuf_end = nullptr;
  outbuf_size = 0;
  writepoint = nullptr;
  outbuf_fd = 0;
}


bool file_loader::resize_outfile(const size_t sz)
{
  off_t checkpoint = writepoint - outbuf;
  if(ftruncate(outbuf_fd, sz) < 0) return false;
  outbuf = (char*)mremap(outbuf, outbuf_size, sz, MREMAP_MAYMOVE);
  if(outbuf == MAP_FAILED)
  {
    close(outbuf_fd);
    null_writepoints();
    return false;
  }
  outbuf_size = sz;
  writepoint = outbuf + checkpoint;
  outbuf_end = outbuf + sz;

  return true;
}


pair<const char*, const char*> file_loader::getline()
{
  if(frontpoint >= inbuf_end) return {nullptr, nullptr};
  if(*frontpoint == '\n') ++frontpoint;
  readpoint = frontpoint;

  frontpoint = (const char*)memchr(readpoint, '\n', inbuf_size - (readpoint - inbuf));
  if(!frontpoint)
    frontpoint = inbuf + inbuf_size;

  return {readpoint, frontpoint};
}


bool file_loader::write(const void* buf, const size_t sz)
{
  if(outbuf_end - writepoint < sz)
  {
    if(!resize_outfile(writepoint - outbuf + sz))
      return false;
  }
  memcpy(writepoint, buf, sz);
  writepoint += sz;
  return true;
}


bool file_loader::put(const char c)
{
  if(writepoint >= outbuf_end)
    if(!resize_outfile(outbuf_size + 256))
      return false;
  *writepoint = c;
  ++writepoint;
  return true;
}


void file_loader::mem_begin(const char* rdbuf, const size_t rdbuf_sz)
{
  inbuf = rdbuf;
  inbuf_end = inbuf + rdbuf_sz;
  inbuf_size = rdbuf_sz;
  readpoint = inbuf;
  frontpoint = inbuf;
  inbuf_fd = 0;
}


void file_loader::mem_terminate()
{
  null_readpoints();
}


void memory_pool::reset()
{
  if(buf != data)
  {
    delete[] buf;
    buf = data;
    bufsize = MEM_POOL_SIZE;
    buf_end = buf + MEM_POOL_SIZE;
  }
  writepoint = buf;
}


bool memory_pool::resize(const size_t sz)
{
  if(sz <= bufsize)
  {
    buf_end -= bufsize - sz;
    bufsize = sz;
    if(writepoint > buf_end)
      writepoint = buf_end;
    return true;
  }

  off_t written_bytes = writepoint - buf;
  char* newbuf = new char[sz];
  if(!newbuf) return false;
  memcpy(newbuf, buf, written_bytes);
  if(buf != data)
    delete[] buf;
  buf = newbuf;
  buf_end = buf + sz;
  bufsize = sz;
  writepoint = buf + written_bytes;
  return true;
}


bool memory_pool::write(const void* rdbuf, const size_t sz)
{
  if(buf_end - writepoint < sz)
    if(!this->resize(writepoint - buf + sz))
      return false;

  memcpy(writepoint, rdbuf, sz);
  writepoint += sz;
  return true;
}


bool memory_pool::put(const char c)
{
  if(writepoint >= buf_end)
    if(!this->resize(bufsize + 256))
      return false;
  *writepoint = c;
  ++writepoint;
  return true;
}


//TODO: substitute by bitset operations
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
  memset(data, 0, DELIMITER_BIT_VEC_SIZE);
}


void iso_8859_1_bitvec::copy_from(const iso_8859_1_bitvec& delim)
{
  memcpy(this->data, delim.data, DELIMITER_BIT_VEC_SIZE);
}


void iso_8859_1_bitvec::copy_from(const string& delim_str)
{
  for(auto reader = delim_str.cbegin(); reader != delim_str.cend(); reader++)
    this->set(*reader, true);
}


void iso_8859_1_bitvec::copy_to(iso_8859_1_bitvec& dst) const
{
  memcpy(dst.data, this->data, DELIMITER_BIT_VEC_SIZE);
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
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos), loader()
{
  constructionLogic();
  delimiters.copy_from(delimitadoresPalabra);
}


Tokenizador::Tokenizador(const Tokenizador& tk) : casosEspeciales(tk.casosEspeciales), pasarAminuscSinAcentos(tk.pasarAminuscSinAcentos), loader()
{
  constructionLogic();
  delimiters.copy_from(tk.delimiters);
}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false), loader()
{
  static const char* delimDefaults = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  const char* i = delimDefaults;
  constructionLogic();
  while(*i != '\0')
  {
    delimiters.set(*i, true);
    ++i;
  }
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
  string str_copy(str);
  if(pasarAminuscSinAcentos)
  {
    for(auto i = str_copy.begin(); i != str_copy.end(); i++)
      *i = normalizeChar(*i);
  }
  loader.mem_begin(str_copy.c_str(), str_copy.length());
  pair<const char*, const char*> tk;

  while(loader.frontpoint < loader.inbuf_end)
  {
    tk = (this->*extractToken)();
    if(tk.first != nullptr)
    {
      tokens.emplace_back(tk.first, tk.second);
      if(*loader.readpoint == ',' || *loader.readpoint == '.')
      {
        tokens.back() = string(1, '0') + tokens.back();
      }
    }
  }
  loader.mem_terminate();
}


bool Tokenizador::Tokenizar(const string& i, const string& f)
{
  return tkFile(i.c_str(), f.c_str());
}


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
  file_loader fl;

  if(!fl.begin(i.c_str())) return false;
  string cur_file;
  auto line = fl.getline();
  while(line.first)
  {
    cur_file = string(line.first, line.second);
    Tokenizar(cur_file, cur_file + ".tk");
    line = fl.getline();
  }

  fl.terminate();
  return true;
}


bool Tokenizador::TokenizarDirectorio(const string& dirAIndexar)
{
  return tkDirectory(dirAIndexar.c_str(), dirAIndexar.length());
}


void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters)
{
  defaultDelimiters();
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
  for(uint8_t i=0; i < ISO_8859_SIZE -1; ++i)
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
  specialDelimiters();
}


bool Tokenizador::CasosEspeciales() const
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


char Tokenizador::normalizeChar(const char c)
{
  int16_t curChar = (int16_t)(uint8_t)c;
  if(curChar >= CAPITAL_START_POINT && curChar <= CAPITAL_END_POINT)
    curChar += Tokenizador::TOLOWER_OFFSET;
  else if(curChar - ACCENT_START_POINT >= 0)
    curChar += Tokenizador::accentRemovalOffsetVec[curChar - ACCENT_START_POINT];

  return (char)curChar;
}


void Tokenizador::skipDelimiters(const bool leaveLastOne)
{
  if(!delimiters.check(*loader.frontpoint)) return;
  while(loader.frontpoint < loader.inbuf_end && delimiters.check(*loader.frontpoint))
    ++loader.frontpoint;

  if(leaveLastOne)
    --loader.frontpoint;
  loader.readpoint = loader.frontpoint;
}


bool Tokenizador::isNumeric(const char c) const
{
  return static_cast<uint8_t>(c) >= Tokenizador::NUMERIC_START_POINT && static_cast<uint8_t>(c) <= Tokenizador::NUMERIC_END_POINT;
}


bool Tokenizador::tkFile(const char* ifile, const char* ofile)
{
  if(!loader.begin(ifile, ofile))
  {
    cerr << "ERROR: No existe el archivo: " << ifile << endl;
    return false;
  }

  if(pasarAminuscSinAcentos)
  {
    for(char* i = const_cast<char*>(loader.inbuf); i < loader.inbuf_end; ++i)
      *i = normalizeChar(*i);
  }
  pair<const char*, const char*> tk;

  while(loader.frontpoint < loader.inbuf_end)
  {
    tk = (this->*extractToken)();
    loader.write(tk.first, tk.second - tk.first);
    loader.put('\n');
  }

  loader.terminate();
  return true;
}


bool Tokenizador::tkDirectory(const char* dir_name, const size_t dir_len)
{
  DIR* dir;
  struct dirent* entry;
  char* entry_name = new char[512];
  size_t entry_len = 256;
  if(!entry_name) return false;

  if((dir = opendir(dir_name)) == nullptr)
  {
    delete[] entry_name;
    return false;
  }

  size_t entry_namlen;
  while((entry = readdir(dir)) != nullptr)
  {
    entry_namlen = strlen(entry->d_name);
    if(entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.')))
      continue;

    // +2 = '/' + '\0'
    if(dir_len + entry_namlen +2 > entry_len)
    {
      delete[] entry_name;
      entry_len = dir_len + entry_namlen +2;
      // << 1 = outfile, +3 = ".tk"
      entry_name = new char[(entry_len << 1) +3];
    }
    memcpy(entry_name, dir_name, dir_len);
    *(entry_name + dir_len) = '/';
    memcpy(entry_name + dir_len +1, entry->d_name, entry_namlen);
    *(entry_name + dir_len + 1 + entry_namlen) = '\0';

    if(entry->d_type == DT_DIR)
    {
      tkDirectory(entry_name, dir_len + entry_namlen +1);
    }
    else
    {
      memcpy(entry_name + entry_len, entry_name, dir_len + entry_namlen +1);
      memcpy(entry_name + entry_len + dir_len + entry_namlen +1, ".tk", 4);
      tkFile(entry_name, entry_name + entry_len);
    }
  }
  delete[] entry_name;
  closedir(dir);
  return true;
}


void Tokenizador::tkAppend(const string& filename, vector<string>& tokens)
{
  loader.begin(filename.c_str());
  tokens.reserve(tokens.size() + (loader.inbuf_size >> 3));
  if(pasarAminuscSinAcentos)
  {
    for(char* i = const_cast<char*>(loader.inbuf); i < loader.inbuf_end; ++i)
      *i = normalizeChar(*i);
  }

  pair<const char*, const char*> tk;
  while(loader.frontpoint < loader.inbuf_end)
  {
    tk = (this->*extractToken)();
    if(tk.first)
    {
      tokens.emplace_back(tk.first, tk.second);
      if(*loader.readpoint == ',' || *loader.readpoint == '.')
        tokens.back() = string(1, '0') + tokens.back();
    }
  }
  loader.terminate();
}


bool Tokenizador::tkDirAppend(const string& dir_name, vector<string>& tokens)
{
  DIR* dir;
  struct dirent* entry;

  if((dir = opendir(dir_name.c_str())) == nullptr)
  {
    cerr << "ERROR: no se ha podido abrir el directorio " << dir_name << " porque no existe." << endl;
    return false;
  }

  while((entry = readdir(dir)) != nullptr)
  {
    if(entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.')))
      continue;

    if(entry->d_type == DT_DIR)
    {
      tkDirAppend(entry->d_name, tokens);
    }
    else
    {
      tkAppend(entry->d_name, tokens);
    }
  }
  return true;
}


pair<const char*, const char*> Tokenizador::extractCommonCaseToken()
{
  skipDelimiters(false);
  loader.readpoint = loader.frontpoint;
  if(loader.frontpoint >= loader.inbuf_end)
  {
    return {nullptr, nullptr};
  }
  while(loader.frontpoint < loader.inbuf_end && !delimiters.check(*loader.frontpoint))
  {
    ++loader.frontpoint;
  }

  return {loader.readpoint, loader.frontpoint};
}


pair<const char*, const char*> Tokenizador::extractSpecialCaseToken()
{
  skipDelimiters(true);
  loader.readpoint = loader.frontpoint;

  loader.frontpoint = decimalTill();
  if(loader.frontpoint)
  {
    if((*loader.readpoint == ',' || *loader.readpoint == '.') && loader.writepoint)
    {
      loader.put(0);
      ++loader.writepoint;
    }
    return {loader.readpoint, loader.frontpoint};
  }

  if(delimiters.check(*loader.readpoint))
    ++loader.readpoint; // Pass delimiter

  loader.frontpoint = multiwordTill();
  if(!loader.frontpoint)
    loader.frontpoint = urlTill();
  if(!loader.frontpoint)
    loader.frontpoint = emailTill();
  if(!loader.frontpoint)
    loader.frontpoint = acronymTill();
  if(!loader.frontpoint)
    return extractCommonCaseToken();

  return {loader.readpoint, loader.frontpoint};
}


const char* Tokenizador::multiwordTill()
{
  const char* reader = loader.readpoint;
  if(!delimiters.check('-') || delimiters.check(*reader))
    return nullptr;

  while(reader < loader.inbuf_end && !delimiters.check(*reader))
    ++reader;
  if(reader == loader.inbuf_end || *reader != '-' || reader +1 == loader.inbuf_end || delimiters.check(*(reader +1)))
    return nullptr;

  reader += 2;
  while(reader < loader.inbuf_end)
  {
    // hyphen allowed
    while(reader < loader.inbuf_end && !delimiters.check(*reader))
      ++reader;

    // no hyphen means end delimiter
    if(reader == loader.inbuf_end || *reader != '-' || reader +1 == loader.inbuf_end || delimiters.check(*(reader +1)))
      return reader;
    ++reader;
  }
  
  return reader;
}


//TODO: make url_delim constexpr
const char* Tokenizador::urlTill()
{
  const char* reader = loader.readpoint; // Not frontpoint because can be null
  iso_8859_1_bitvec url_delim;
  url_delim.reset();
  url_delim.copy_from("_:/.?&-=#@");

  if(
    reader +4 >= loader.inbuf_end 
    || (!url_delim.check(*(reader +4)) && delimiters.check(*(reader +4)))
    || !(
      ((*reader) == 'f' 
       && (*(reader +1)) == 't' 
       && (*(reader +2)) == 'p' 
       && *(reader +3) == ':')
      || ((*reader) == 'h' 
        && (*(reader +1)) == 't' 
        && (*(reader +2)) == 't' 
        && (*(reader +3)) == 'p'
        && (*(reader +4) == ':'
          || (*(reader +4) == 's' && reader +5 < loader.inbuf_end && *(reader +5) == ':')))))
  {
    return nullptr;
  }
  else reader += 4;

  // The reader must point to ':'
  if(*(reader -1) == ':') --reader;
  else if((*reader) == 's') ++reader;

  if(reader +1 >= loader.inbuf_end || (delimiters.check(*(reader +1)) && !url_delim.check(*(reader +1))))
    return nullptr;
  while(reader < loader.inbuf_end)
  {
    if(delimiters.check(*reader) && !url_delim.check(*reader))
      return reader;
    ++reader;
  }

  return reader;
}


const char* Tokenizador::emailTill()
{
  if(!delimiters.check('@') || *loader.readpoint == '@')
    return nullptr;

  const char* reader = loader.readpoint;
  bool found_at_sign = false;

  while(reader < loader.inbuf_end && !found_at_sign)
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
  while(reader < loader.inbuf_end)
  {
    if(*reader == '@')
      return nullptr; // Not allowed second @
    if(sufix_delimiters.check(*reader)) 
    {
      if(delimiters.check(*(reader -1)) || reader +1 == loader.inbuf_end || delimiters.check(*(reader +1)))
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

  const char* reader = loader.readpoint;
  while(reader < loader.inbuf_end)
  {
    if(
        *reader == '.' && (reader +1 == loader.inbuf_end || delimiters.check(*(reader +1)))
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
  const char* reader = loader.readpoint;
  const char *dot = nullptr, *comma = nullptr;

  if(*reader == '.')
  {
    dot = reader;
    ++reader;
  }
  else if(*reader == ',')
  {
    comma = reader;
    ++reader;
  }

  if(!isNumeric(*reader))
    return nullptr;
  else ++reader;

  // Guaranteed at least one numeric char
  while(reader < loader.inbuf_end)
  {
    if(*reader == '.')
    {
      dot = reader;
      if(comma && dot && comma == dot -1) return comma;
      if(reader +1 == loader.inbuf_end) return reader;
    }
    else if(*reader == ',')
    {
      comma = reader;
      if(dot && comma && dot == comma -1) return dot;
      if(reader +1 == loader.inbuf_end) return reader;
    }
    else if(delimiters.check(*reader))
    {
      if(comma == reader -1 || dot == reader -1) // Cut the separator if useless
        --reader;
      return reader;
    }
    else if(!isNumeric(*reader))
    {
      return nullptr;
    }
    ++reader;
  }
  return reader;
}


void Tokenizador::constructionLogic()
{
  defaultDelimiters();
  
  if(casosEspeciales)
    extractToken = &Tokenizador::extractSpecialCaseToken;
  else
    extractToken = &Tokenizador::extractCommonCaseToken;
}


void Tokenizador::defaultDelimiters()
{
  delimiters.reset();
  delimiters.set('\0', true);
  delimiters.set('\n', true);
  specialDelimiters();
}


void Tokenizador::specialDelimiters()
{
  if(casosEspeciales)
    delimiters.set(0x20 /*empty space*/, true);
  else
    delimiters.set(0x20, false);
}
