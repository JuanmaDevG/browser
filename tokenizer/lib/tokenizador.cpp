#include "tokenizador.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>


#define is_numeric(n) (n >= NUMERIC_START_POINT && n <= NUMERIC_END_POINT)


bitset<ISO_8859_SIZE> Tokenizador::url_delimiters;
bitset<ISO_8859_SIZE> Tokenizador::email_delimiters;
const unsigned char Tokenizador::iso8859_norm_table[256];


ostream& operator<<(ostream& os, const Tokenizador& tk)
{
  cout << "DELIMITADORES: ";
  for(size_t i=0; i < ISO_8859_SIZE; ++i)
  {
    if(tk.delimiters[i]) cout << (char)i;
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
  static const unsigned char auto_delimiters[] = ",;:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
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


//TODO: outbuf size update to support decimal numbers
void Tokenizador::Tokenizar(const string& str, list<string>& tokens)
{
  tokens.clear();
  const size_t inbuf_size = str.size();
  const unsigned char *const inbuf = (const unsigned char*)str.data();
  unsigned char *const outbuf = new unsigned char[inbuf_size + (inbuf_size >> 1)]; //outbut_size = inbuf_size * 1.5

  const size_t written_bytes = tokenize_buffer(inbuf, inbuf_size, outbuf);

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
  int o_fd = open(f.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if(o_fd < 0) {
    cerr << "No se pudo crear el fichero: " << f << endl;
    return false;
  }

  struct stat fileinfo;
  fstat(i_fd, &fileinfo);
  close(i_fd);
  const unsigned char *const inbuf = (const unsigned char*)mmap(nullptr, fileinfo.st_size, PROT_READ, MAP_SHARED, i_fd, 0);
  if(inbuf == MAP_FAILED) {
    cerr << "No se ha podido mapear el fichero de entrada: " << i << endl;
    return false;
  }
  madvise(const_cast<unsigned char*>(inbuf), fileinfo.st_size, MADV_SEQUENTIAL | MADV_WILLNEED);

  ftruncate(o_fd, (off_t)fileinfo.st_size + (fileinfo.st_size >> 1)); // outbuf_size = inbuf_size * 1.5
  unsigned char *const outbuf = (unsigned char*)mmap(nullptr, fileinfo.st_size, PROT_WRITE, MAP_SHARED, o_fd, 0);
  if(outbuf == MAP_FAILED)
  {
    cerr << "No se pudo mapear el fichero de salida: " << f << endl;
    close(o_fd);
    return false;
  }
  madvise(outbuf, fileinfo.st_size, MADV_SEQUENTIAL);

  const size_t written_bytes = tokenize_buffer(inbuf, fileinfo.st_size, outbuf);

  munmap(const_cast<unsigned char*>(inbuf), fileinfo.st_size);
  msync(outbuf, fileinfo.st_size, MS_ASYNC);
  munmap(outbuf, fileinfo.st_size);
  ftruncate(o_fd, written_bytes);
  close(o_fd);
  return true;
}


bool Tokenizador::TokenizarListaFicheros(const string& i)
{
  int r_fd = open(i.c_str(), O_RDONLY);
  if(r_fd < 0) return false;
  struct stat r_info;
  fstat(r_fd, &r_info);
  const unsigned char *const rbuf = (const unsigned char*)mmap(nullptr, r_info.st_size, PROT_READ, MAP_SHARED, r_fd, 0);
  close(r_fd);
  if(rbuf == MAP_FAILED)
    return false;
  madvise(const_cast<unsigned char*>(rbuf), r_info.st_size, MADV_SEQUENTIAL);
  const unsigned char *const rbuf_end = rbuf + r_info.st_size;
  string inpath, outpath;

  const unsigned char *backpoint = rbuf, *frontpoint = rbuf;
  while(backpoint < rbuf_end)
  {
    while(*frontpoint != '\n') ++frontpoint;
    inpath.assign(backpoint, frontpoint);
    outpath.assign(backpoint, frontpoint).append(".tk");
    ++frontpoint;
    backpoint = frontpoint;
    Tokenizar(inpath, outpath);
  }

  munmap(const_cast<unsigned char*>(rbuf), r_info.st_size);
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
      TokenizarDirectorio(string(path_buf, dir_len + nlen + 1));
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


extern inline void Tokenizador::default_delimiters()
{
  delimiters.set((size_t)'\0');
  delimiters.set((size_t)'\n');
  delimiters.set((size_t)'\r');
  if(casosEspeciales) delimiters.set((size_t)' ');
  
  // Static delimiters for special cases
  if(!Tokenizador::special_delimiters_done)
    Tokenizador::initialize_special_delimiters();
}


extern inline void Tokenizador::add_delimiters(const unsigned char *delim, const size_t n)
{
  for(size_t i=0; i < n; ++i)
    delimiters.set((size_t)delim[i]);
}


extern inline void Tokenizador::normalize(unsigned char *buf, const unsigned char *const buf_end) const
{
  while(buf < buf_end)
  {
    *buf = iso8859_norm_table[*buf];
    ++buf;
  }
}


size_t Tokenizador::tokenize_buffer(const unsigned char *readpoint, const size_t r_bufsize, unsigned char *writepoint) const
{
  const unsigned char *const inbuf_end = readpoint + r_bufsize;
  const unsigned char *const outbuf_begin = writepoint;

  if(casosEspeciales)
  {
    unsigned char c1, c2;
    unsigned char cmpbuf[6];
    off_t rd_offset, buf_delta;
    while(readpoint < inbuf_end)
    {
      //url_precondition:
      rd_offset = 0; buf_delta = inbuf_end - readpoint;
      memcpy(cmpbuf, readpoint, (buf_delta < 6 ? buf_delta : 6));
      normalize(cmpbuf, cmpbuf + 6);
      if(buf_delta >= 5 && strncmp((const char*)cmpbuf, "http:", 5) == 0)
        rd_offset = 5;
      else if(buf_delta >= 6 && strncmp((const char*)cmpbuf, "https:", 6) == 0)
        rd_offset = 6;
      else if(buf_delta >= 4 && strncmp((const char*)cmpbuf, "ftp:", 4) == 0)
        rd_offset = 4;

      //url:
      if(rd_offset > 0) {
        memcpy(writepoint, readpoint, (size_t)rd_offset);
        readpoint += rd_offset;
        writepoint += rd_offset;
        while(readpoint < inbuf_end) {
          c1 = *readpoint++;
          if(delimiters[c1] && !Tokenizador::url_delimiters[c1])
            break;
          *writepoint++ = c1;
        }
      }

      c1 = *readpoint++;
      //multiword:
      if(delimiters['-'] && !delimiters[c1]) {
        do {
          *writepoint++ = c1;
          c1 = *readpoint++;
          if(c1 == '-') {
            if(readpoint == inbuf_end || delimiters[(c2 = *readpoint++)])
              break;
            *writepoint++ = c1;
            c1 = c2;
          }
        } while(readpoint < inbuf_end);
      }

      //email:
      else if(!delimiters[c1] && delimiters['@'])
      {
        //TODO: watch the specific case specification
        //TODO: second @ not allowed, store @pos and do \n and return to @pos+1
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

      //acronym:
      else if(delimiters['.'] && !delimiters[c1])
      {
        *writepoint++ = c1;
        while(readpoint < inbuf_end) {
          c1 = *readpoint++;
          if(c1 == '.') {
            if(readpoint == inbuf_end) break;
            c2 = *readpoint++;
            if(delimiters[c2]) break;
            *writepoint++ = c1;
            *writepoint++ = c2;
            continue;
          }
          *writepoint++ = c1;
        }
      }

      //number:
      else if(/*number condition*/false) {
        //TODO: number case
      }

      //normal_word:
      else {
        while(delimiters[c1] && readpoint < inbuf_end);
        while(!delimiters[c1] && readpoint < inbuf_end) {
          *writepoint++ = c1;
          c1 = *readpoint++;
        }
      }
      *writepoint++ = '\n';
    }
  }
  else // no special
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
    normalize(const_cast<unsigned char*>(outbuf_begin), outbuf_begin + r_bufsize);

  return writepoint - outbuf_begin; // written bytes
}


bool Tokenizador::special_delimiters_done = false;

void Tokenizador::initialize_special_delimiters()
{
  static const unsigned char raw_url_delim[] = "_:/.?&-=#@";
  static const unsigned char raw_email_delim[] = ".-_";
  size_t N = sizeof(raw_url_delim) -1;

  for(size_t i = 0; i < N; ++i)
    Tokenizador::url_delimiters.set(raw_url_delim[i]);

  N = sizeof(raw_email_delim) -1;
  for(size_t i=0; i < N; ++i)
    Tokenizador::email_delimiters.set(raw_email_delim[i]);

  special_delimiters_done = true;
}
