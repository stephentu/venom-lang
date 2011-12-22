#include <analysis/symboltable.h>

using namespace std;

namespace venom {
namespace analysis {

Symbol* SymbolTable::createSymbol(const string&     name,
                                  InstantiatedType* type) {
  Symbol *sym = new Symbol(name, type);
  symbolContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::createFuncSymbol(const string&        name,
                              const vector<Type*>& params,
                              Type*                returnType) {
  FuncSymbol *sym = new FuncSymbol(name, params, returnType);
  funcContainer.insert(name, sym);
  return sym;
}

ClassSymbol*
SymbolTable::createClassSymbol(const string& name,
                               Type*         type) {
  ClassSymbol *sym = new ClassSymbol(name, type);
  classContainer.insert(name, sym);
  return sym;
}

}
}
