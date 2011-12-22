#ifndef VENOM_ANALYSIS_SYMBOLTABLE_H
#define VENOM_ANALYSIS_SYMBOLTABLE_H

#include <map>
#include <string>
#include <vector>

#include <analysis/symbol.h>
#include <util/stl.h>

namespace venom {
namespace analysis {

class SymbolTable {
private:
  /** Create a child symbol table */
  SymbolTable(SymbolTable* parent) : parent(parent) {}
public:
  /** Create the root symbol table */
  SymbolTable() : parent(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

  inline SymbolTable* newChildScope() {
    SymbolTable *child = new SymbolTable(this);
    children.push_back(child);
    return child;
  }

  Symbol* createSymbol(const std::string& name,
                       InstantiatedType*  type);

  FuncSymbol*
  createFuncSymbol(const std::string& name,
                   const std::vector<Type*>& params,
                   Type*                     returnType);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    Type*              type);

private:
  SymbolTable*              parent;
  std::vector<SymbolTable*> children;

  template <typename S>
  struct container {
    std::map<std::string, S> map;
    std::vector<S>           vec;
    inline void insert(const std::string& name,
                       const S&           elem) {
      map[name] = elem;
      vec.push_back(elem);
    }
  };

  /** Containers for each kind of symbol */
  container<Symbol*>      symbolContainer;
  container<FuncSymbol*>  funcContainer;
  container<ClassSymbol*> classContainer;

};

}
}

#endif /* VENOM_ANALYSIS_SYMBOLTABLE_H */
