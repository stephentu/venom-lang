#include <cassert>

#include <analysis/symboltable.h>
#include <ast/expression/node.h>

using namespace std;

namespace venom {
namespace analysis {

Symbol*
SymbolTable::createSymbol(const string&     name,
                          InstantiatedType* type) {
  Symbol *sym = new Symbol(name, type);
  symbolContainer.insert(name, sym);
  return sym;
}

Symbol*
SymbolTable::findSymbol(const string& name, bool recurse) {
  Symbol *ret = NULL;
  symbolContainer.find(ret, name, recurse);
  return ret;
}

FuncSymbol*
SymbolTable::createFuncSymbol(const string&        name,
                              const vector<Type*>& params,
                              Type*                returnType) {
  FuncSymbol *sym = new FuncSymbol(name, params, returnType);
  funcContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::findFuncSymbol(const string& name, bool recurse) {
  FuncSymbol *ret = NULL;
  funcContainer.find(ret, name, recurse);
  return ret;
}

ClassSymbol*
SymbolTable::createClassSymbol(const string& name,
                               Type*         type) {
  ClassSymbol *sym = new ClassSymbol(name, type);
  classContainer.insert(name, sym);
  return sym;
}

ClassSymbol*
SymbolTable::findClassSymbol(const string& name, bool recurse) {
  ClassSymbol *ret = NULL;
  classContainer.find(ret, name, recurse);
  return ret;
}

ClassSymbol*
SymbolTable::findClassSymbol(const ast::ParameterizedTypeString* name,
                             bool recurse,
                             const ast::ParameterizedTypeString*& failed_type,
                             bool& wrong_params) {
  failed_type = NULL; wrong_params = false;

  ClassSymbol *ret = findClassSymbol(name->name, recurse);
  if (ret) {
    // num param check
    if (name->params.size() != ret->getType()->getParams()) {
      failed_type = name;
      wrong_params = true;
      return NULL;
    }
    bool fail = false;
    for (ast::TypeStringVec::const_iterator it = name->params.begin();
         it != name->params.end(); ++it) {
      ClassSymbol *cs = findClassSymbol(*it, true, failed_type, wrong_params);
      if (!cs) {
        assert(failed_type);
        fail = true;
        break;
      }
    }
    return fail ? NULL : ret;
  } else {
    failed_type = name;
    return NULL;
  }
}

}
}
