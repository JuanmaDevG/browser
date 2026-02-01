#include "tokenizador.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>


constexpr bitset<ISO_8859_SIZE> get_url_delimiters() {
  bitset<ISO_8859_SIZE> result;
  const unsigned char delim[] = "_:/.?&-=#@";
  const size_t N = sizeof(delim) -1;

  for(size_t i = 0; i < N; i++) {
    result.set(delim[i]);
  }
  return result;
}


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


ostream& operator<<(ostream& os, const Tokenizador& tk)
{
  cout << "DELIMITADORES: ";
  for(size_t i=0; i < ISO_8859_SIZE; ++i)
  {
    if(tk.delimiters[i])
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
  }

  cout << " TRATA DE CASOS ESPECIALES: " << tk.casosEspeciales 
    << " PASAR A MINUSCULAS Y SIN ACENTOS: " << tk.pasarAminuscSinAcentos
    << flush;

  return os;
}


Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool casosEspeciales, const bool minuscSinAcentos) :
  casosEspeciales(casosEspeciales), pasarAminuscSinAcentos(minuscSinAcentos), delimiters()
{
  default_delimiters();
  add_delimiters(
      reinterpret_cast<const unsigned char*>(delimitadoresPalabra.data()),
      delimitadoresPalabra.size());
}


Tokenizador::Tokenizador(const Tokenizador& tk) :
  casosEspeciales(tk.casosEspeciales),
  pasarAminuscSinAcentos(tk.pasarAminuscSinAcentos),
  delimiters(tk.delimiters) {}


Tokenizador::Tokenizador() : casosEspeciales(true), pasarAminuscSinAcentos(false), delimiters()
{
  default_delimiters();
  static const unsigned char[] auto_delimiters = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
  add_delimiters(auto_delimiters, sizeof(auto_delimiters) -1);
}


Tokenizador::~Tokenizador() {}


Tokenizador& Tokenizador::operator=(const Tokenizador& tk)
{
  casosEspeciales = tk.casosEspeciales;
  pasarAminuscSinAcentos = tk.pasarAminuscSinAcentos;
  delimiters = tk.delimiters;
  return *this;
}


void Tokenizador::Tokenizar(const string& str, list<string>& tokens)
{
  tokens.clear();
  const size_t inbuf_size = str.size();
  const unsigned char *const inbuf = (const unsigned char*)str.data();
  unsigned char *const outbuf = new unsigned char[inbuf_size];

  const size_t written_bytes = tokenize_buffer(inbuf, outbuf, inbuf_size);

  const unsigned char *const outbuf_end = outbuf + written_bytes;
  const unsigned char *tbegin = outbuf, *tend = outbuf;
  while(tbegin < outbuf_end)
  {
    while(*tend != '\n')
      tend++;
    tokens.emplace_back(tbegin, tend);
    ++tend;
    tbegin = tend;
  }
  delete[] outbuf;
}


bool Tokenizador::Tokenizar(const string& i, const string& f)
{
  int i_fd = open(i.c_str(), O_RDONLY);
  if(i_fd < 0) {
    cerr << "No se ha encontrado el fichero: " << i << endl;
    return false;
  }
  int o_fd = open(o.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if(o_fd < 0) {
    cerr << "No se pudo crear el fichero: " << f << endl;
    return false;
  }

  struct stat fileinfo;
  fstat(i_fd, &fileinfo);
  close(i_fd);
  const unsigned char *const inbuf = (const unsigned char*)mmap(nullptr, fileinfo.st_size, PROT_READ, MAP_SHARED, inbuf_fd, 0);
  if(inbuf == MAP_FAILED) {
    cerr << "No se ha podido mapear el fichero de entrada: " << i << endl;
    return false;
  }
  madvise(inbuf, fileinfo.st_size, MADV_SEQUENTIAL | MADV_WILLNEED);

  ftruncate(o_fd, (off_t)fileinfo.st_size);
  unsigned char *const outbuf = (const unsigned char*)mmap(nullptr, fileinfo.st_size, PROT_WRITE, MAP_SHARED, o_fd, 0);
  if(outbuf == MAP_FAILED)
  {
    cerr << "No se pudo mapear el fichero de salida: " << f << endl;
    close(o_fd);
    return false;
  }
  madvise(outbuf, fileinfo.st_size, MADV_SEQUENTIAL);

  const size_t written_bytes = tokenize_buffer(inbuf, outbuf, fileinfo.st_size);

  munmap(inbuf, fileinfo.st_size);
  msync(outbuf, fileinfo.st_size, MS_ASYNC);
  munmap(outbuf, fileinfo.st_size);
  ftruncate(o_fd, written_bytes)
  close(o_fd);
  return true;
}


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
  int r_fd = open(i.c_str(), O_RDONLY);
  if(r_fd < 0) return false;
  struct stat r_info;
  fstat(r_fd, &r_info);
  const unsigned char *const rbuf = mmap(nullptr, r_info.st_size, PROT_READ, MAP_SHARED, r_fd, 0);
  close(r_fd);
  if(rbuf == MAP_FAILED)
    return false;
  madvise(rbuf, r_info.st_size, MADV_SEQUENTIAL);
  const unsigned char *const rbuf_end = rbuf + r_info.st_size;
  string inpath, outpath;

  const unsigned char* backpoint = rbuf, frontpoint = rbuf;
  while(backpoint < rbuf_end)
  {
    while(*frontpoint != '\n') ++frontpoint;
    inpath.assign(backpoint, frontpoint);
    outpath.assign(backpoint, frontpoint).append(".tk");
    ++frontpoint;
    backpoint = frontpoint;
    tokenizar(inpath, outpath);
  }

  munmap(rbuf, r_info.st_size);
  return true;
}


bool Tokenizador::TokenizarDirectorio(const string& dirAIndexar)
{
  size_t dir_len = dirAIndexar.size();
  const char *const dir_name = dirAIndexar.c_str();
  DIR* dir = opendir(dir_name);
  if (!dir) {
    cerr << "No se ha encontrado el directorio: " << dirAIndexar << endl;
    return false;
  }

  char path_buf[8192];
  memcpy(path_buf, dir_name, dir_len);
  path_buf[dir_len] = '/';
  
  dirent* entry;
  while ((entry = readdir(dir))) {
    const char* dname = entry->d_name;
    if (dname[0] == '.' && (!dname[1] || (dname[1] == '.' && !dname[2])))
      continue;

    size_t nlen = strlen(dname);
    //if (dir_len + nlen >= 4094) continue;
    memcpy(path_buf + dir_len + 1, dname, nlen + 1);

    if (entry->d_type == DT_DIR) {
      TokenizarDirectorio(path_buf, dir_len + nlen + 1);
    } else {
      char* out = path_buf + 4096;
      memcpy(out, path_buf, dir_len + nlen + 2);
      memcpy(out + dir_len + nlen + 1, ".tk", 4);
      Tokenizar(path_buf, out);
    }
  }
  
  closedir(dir);
  return true;
}


void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters)
{
  delimiters.reset();
  default_delimiters();
  add_delimiters(
      reinterpret_cast<const unsigned char*>(nuevoDelimiters.data()),
      nuevoDelimiters.size());
}


void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{
  add_delimiters(
      reinterpret_cast<const unsigned char*>(nuevoDelimiters.data()),
      nuevoDelimiters.size());
}


string Tokenizador::DelimitadoresPalabra() const
{
  string result;
  result.reserve(delimiters.count() +1);

  for(size_t i=0; i < ISO_8859_SIZE -1; ++i)
    if(delimiters[i])
      result.push_back((char)i);

  return result;
} 


void Tokenizador::CasosEspeciales(const bool nuevoCasosEspeciales)
{
  casosEspeciales = nuevoCasosEspeciales;
  if(casosEspeciales) delimiters.set((size_t)' ');
  else delimiters.reset((size_t)' ');
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


void Tokenizador::default_delimiters()
{
  delimiters.set((size_t)'\0');
  delimiters.set((size_t)'\n');
  if(casosEspeciales) delimiters.set((size_t)' ');
}


void Tokenizador::add_delimiters(const char *restrict delim, const size_t n)
{
  for(size_t i=0; i < n; ++i)
    delimiters.set((size_t)delim[i]);
}


void Tokenizador::normalize(unsigned char *restrict buf, const unsigned char *const restrict buf_end) const
{
  while(begin < end)
  {
    *begin = iso8859_norm_table[*begin];
    ++begin;
  }
}


size_t Tokenizador::tokenize_buffer(const unsigned char *readpoint, unsigned char *writepoint, const size_t min_bufsize) const
{
  const unsigned char *const inbuf_end = readpoint + min_bufsize;
  const unsigned char *const outbuf_begin = writepoint;

  //TODO: probably check for buffer resize

  if(casosEspeciales)
  {
    unsigned char c1, c2;
    while(readpoint < inbuf_end)
    {
      c1 = *readpoint++;
      if(delimiters[c1])
        continue;

      normal_word:
      *writepoint++ = c1;
      while(readpoint < writepoint && !delimiters[(c1 = *readpoint++)])
        *writepoint++ = c1;
      if(readpoint == inbuf_end) break;

      multiword:
      if(c1 == '-' && !delimiters[(c2 = *readpoint++)]) {
        *writepoint++ = c1;
        *writepoint++ = c2;
        while(readpoint < inbuf_end)
        {
          c1 = *readpoint++;
          if(!delimiters[c1])
            *writepoint++ = c1;
          else if(c1 == '-' && readpoint < inbuf_end && !delimiters[(c2 = *readpoint++)]) {
            *writeponit++ = c1;
            *writepoint++ = c2;
          }
          else break; 
        }
      }
      else 
      {
        //TODO: dice el enunciado que las url's son el primer caso que se comprueba, mirar bien
        url:
        constexpr bitset<ISO_8859_SIZE> url_delim = get_url_delimiters();
        off_t r_offset = 0, buf_delta = outbuf + min_fsize - writepoint;
        if(buf_delta >= 5 && strncmp(readpoint, "http:", 5) == 0)
          r_offset = 5;
        else if(buf_delta >= 6 && strncmp(readpoint, "https:", 6) == 0)
          r_offset = 6;
        else if(buf_delta >= 4 && strncmp(readpoint, "ftp:", 4) == 0)
          r_offset = 4;

        if(r_offset > 0) {
          memcpy(writepoint, readpoint, (size_t)r_offset);
          readpoint += r_offset;
          writepoint += r_offset;
          while(readpoint < inbuf_end) {
            c1 = *readpoint++;
            if(delimiters[c1] && !url_delim[c1])
              break;
            *writepoint++ = c1;
          }
        }
      }
    }
  }
  else // no special cases
  {
    bool got_char = false;
    while(readpoint < inbuf_end)
    {
      if(delimiters[*readpoint]) {
        ++readpoint;
        if(got_char) {
          got_char = false;
          *writepoint = '\n';
          ++writepoint;
        }
        continue;
      }
      got_char = true;
      *writepoint = *readpoint;
      ++readpoint;
      ++writepoint;
    }
  }
  if(pasarAminuscSinAcentos)
    normalize(outbuf, outbuf_end);

  return writepoint - outbuf_begin; // written bytes
}


bool Tokenizador::isNumeric(const char c) const
{
  return static_cast<uint8_t>(c) >= Tokenizador::NUMERIC_START_POINT && static_cast<uint8_t>(c) <= Tokenizador::NUMERIC_END_POINT;
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
  if(*loader.readpoint == '\0')
  {
    loader.frontpoint = loader.readpoint;
    return {nullptr, nullptr};
  }

  loader.frontpoint = multiwordTill();
  if(!loader.frontpoint)
    loader.frontpoint = urlTill();
  if(!loader.frontpoint)
    loader.frontpoint = emailTill();
  if(!loader.frontpoint)
    loader.frontpoint = acronymTill();

  if(loader.frontpoint)
    return {loader.readpoint, loader.frontpoint};
  else
    loader.frontpoint = loader.readpoint;

  return extractCommonCaseToken();
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

  // Guarantee at least one numeric char
  if(*reader == '.' || *reader == ',') ++reader;
  else if(delimiters.check(*reader)) ++loader.readpoint, ++reader;

  if(!isNumeric(*reader)) return nullptr;
  ++reader;

  while(reader < loader.inbuf_end)
  {
    if(delimiters.check(*reader))
    {
      if(!(*reader == '.' || *reader == ',') || delimiters.check(*(reader +1)))
        return reader;
    }
    else if(!isNumeric(*reader))
      return nullptr;

    ++reader;
  }
  return reader;
}
