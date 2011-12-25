#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>

#include <ast/expression/node.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

bool
SymbolTable::isDefined(const string& name, unsigned int type, bool recurse) {
  return findBaseSymbol(name, type, recurse);
}

BaseSymbol*
SymbolTable::findBaseSymbol(const string& name, unsigned int type, bool recurse) {
  assert(type & (Location | Function | Class));
  BaseSymbol *ret = NULL;
  if ((type & Location) && (ret = findSymbol(name, recurse))) return ret;
  if ((type & Function) && (ret = findFuncSymbol(name, recurse))) return ret;
  if ((type & Class) && (ret = findClassSymbol(name, recurse))) return ret;
  return NULL;
}

Symbol*
SymbolTable::createSymbol(const string&     name,
                          InstantiatedType* type) {
  Symbol *sym = new Symbol(name, this, type);
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
SymbolTable::createFuncSymbol(const string&                    name,
                              const vector<InstantiatedType*>& params,
                              InstantiatedType*                returnType) {
  FuncSymbol *sym = new FuncSymbol(name, this, params, returnType);
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
                               SymbolTable*  classTable,
                               Type*         type) {
  ClassSymbol *sym = new ClassSymbol(name, this, classTable, type);
  type->setClassSymbol(sym);
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
SymbolTable::findClassSymbol(const ParameterizedTypeString* name,
                             bool recurse,
                             const ParameterizedTypeString*& failed_type,
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
    for (TypeStringVec::const_iterator it = name->params.begin();
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

ClassSymbol*
SymbolTable::findClassSymbolOrThrow(const ParameterizedTypeString* type,
                                    bool recurse) {
  const ParameterizedTypeString* failed_type;
  bool wrong_params;
  ClassSymbol *parsym = findClassSymbol(type, true, failed_type, wrong_params);
  if (!parsym) {
    assert(failed_type);
    if (wrong_params) {
      throw SemanticViolationException(
          "Wrong number of type parameters given to " + failed_type->name);
    } else {
      throw SemanticViolationException(
          "Type " + failed_type->name + " not defined");
    }
  }
  return parsym;
}

}
}
