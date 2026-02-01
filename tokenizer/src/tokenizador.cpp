#include "tokenizador.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>


//TODO: look if something useful for resize_purposes
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


//TODO: update to tokenize_buffer
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
    while(*tend != '\n' && tend < outbuf_end)
      ++tend;
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
  
  // Static delimiters for special cases
  if(!Tokenizador::special_delimiters_done)
    Tokenizador::initialize_special_delimiters();
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
      skip_delimiters:
      c1 = *readpoint++;
      if(delimiters[c1])
        continue;

      url_preconditions:
      unsigned char cmpbuf[6];
      off_t r_offset = 0, buf_delta = outbuf + min_fsize - writepoint;
      memcpy(cmpbuf, readpoint, (buf_delta < 6 ? buf_delta : 6));
      normalize(cmpbuf, cmpbuf + 6);
      if(buf_delta >= 5 && strncmp(cmpbuf, "http:", 5) == 0)
        r_offset = 5;
      else if(buf_delta >= 6 && strncmp(cmpbuf, "https:", 6) == 0)
        r_offset = 6;
      else if(buf_delta >= 4 && strncmp(cmpbuf, "ftp:", 4) == 0)
        r_offset = 4;
      url:
      if(r_offset > 0) {
        memcpy(writepoint, readpoint, (size_t)r_offset);
        readpoint += r_offset;
        writepoint += r_offset;
        while(readpoint < inbuf_end) {
          c1 = *readpoint++;
          if(delimiters[c1] && !url_delimiters[c1])
            break;
          *writepoint++ = c1;
        }
        continue;
      }

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
      email:
      else if(!delimiters[(c1 = *readpoint++)] && delimiters['@'])
      {
        bool valid_email = true;
        *writepoint++ = c1;
        while(readpoint < inbuf_end) {
          if((c1 = *readpoint++) == '@') {
            *writepoint++ = c1;
            break;
          }
          if(delimiters[c1]) {
            valid_email = false;
            break;
          }
          *writepoint++ = c1;
        }
        if(!valid_email) continue;
        while(readpoint < inbuf_end) {
          c1 = *readpoint++;
          if(delimiters[c1] && !email_delimiters[c1])
            break;
          *writepoint++ = c1;
        }
      }
      acronym:
      else if(/**/)
      {
      }
      number:
      else {
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


static void Tokenizador::initialize_special_delimiters()
{
  static const unsigned char raw_url_delim[] = "_:/.?&-=#@";
  static const unsigned char raw_email_delim[] = ".-_";
  size_t N = sizeof(raw_url_delim) -1;

  for(int i = 0; i < N; ++i)
    Tokenizador::url_delimiters.set(raw_url_delim[i]);

  N = sizeof(raw_email_delim) -1;
  for(int i=0; i < N; ++i)
    Tokenizador::email_delimiters.set(raw_email_delim[i]);

  special_delimiters_done = true;
}


//TODO: pass it (and more conditions) to a macro
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
