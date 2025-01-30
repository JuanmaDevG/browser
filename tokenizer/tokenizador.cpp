#include <tokenizador.h>

ostream& operator<<(ostream& os, const Tokenizador& tk)
{

}

Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos)
{

}

Tokenizador::Tokenizador(const Tokenizador&)
{

}

Tokenizador::Tokenizador()
{

}	

Tokenizador::~Tokenizador()
{

}

Tokenizador::Tokenizador& operator=(const Tokenizador&)
{

}

void Tokenizador::Tokenizar(const string& str, list<string>& tokens) const
{

}

bool Tokenizador::Tokenizar(const string& i, const string& f) const
{

} 

bool Tokenizador::TokenizarListaFicheros(const string& i) const
{

} 

bool Tokenizador::TokenizarDirectorio(const string& i) const
{

} 

void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters)
{

} 

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters)
{

} // 

string Tokenizador::DelimitadoresPalabra() const
{

} 

void Tokenizador::CasosEspeciales(const bool& nuevoCasosEspeciales)
{

}

void Tokenizador::PasarAminuscSinAcentos(const bool& nuevoPasarAminuscSinAcentos)
{

}

bool Tokenizador::PasarAminuscSinAcentos()
{

}
