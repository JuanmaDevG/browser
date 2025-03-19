#include <iostream> 
#include <string>
#include <list> 
#include "tokenizador.h"

using namespace std;

int
main(void)
{
  Tokenizador tk("$%&/()=._-@", true, true);
  if(!tk.TokenizarDirectorio("corpus"))
    cout << "ALGO VA MAL" << endl;
}
