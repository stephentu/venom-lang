#ifndef VENOM_ANALYSIS_SYMBOLTABLE_H
#define VENOM_ANALYSIS_SYMBOLTABLE_H

#include <map>
#include <string>
#include <vector>

#include <analysis/symbol.h>
#include <util/macros.h>
#include <util/stl.h>

namespace venom {

namespace ast {
  /** Forward decl */
  struct ParameterizedTypeString;
}

namespace analysis {

class SymbolTable {
private:
  /** Create a child symbol table */
  SymbolTable(SymbolTable* parent)
    : parent(parent),
      symbolContainer(SAFE_ADDR(parent, symbolContainer)),
      funcContainer(SAFE_ADDR(parent, funcContainer)),
      classContainer(SAFE_ADDR(parent, classContainer)) {}
public:
  /** Create the root symbol table */
  SymbolTable()
    : parent(NULL),
      symbolContainer(NULL),
      funcContainer(NULL),
      classContainer(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

  inline SymbolTable* newChildScope() {
    SymbolTable *child = new SymbolTable(this);
    children.push_back(child);
    return child;
  }

  Symbol*
  createSymbol(const std::string& name,
               InstantiatedType*  type);

  Symbol*
  findSymbol(const std::string& name, bool recurse);

  FuncSymbol*
  createFuncSymbol(const std::string&        name,
                   const std::vector<Type*>& params,
                   Type*                     returnType);

  FuncSymbol*
  findFuncSymbol(const std::string& name, bool recurse);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    Type*              type);

  ClassSymbol*
  findClassSymbol(const std::string& name, bool recurse);

  /**
   * Recurse only applies to the outer type, not the parameterized types
   *
   * The parameterized types will be recusively checked regardless of
   * recurse.
   */
  ClassSymbol*
  findClassSymbol(const ast::ParameterizedTypeString* name,
                  bool recurse,
                  const ast::ParameterizedTypeString*& failed_type,
                  bool& wrong_params);

private:
  SymbolTable*              parent;
  std::vector<SymbolTable*> children;

  template <typename S>
  class container {
  public:
    container(container<S>* parent) : parent(parent) {}
    typedef std::map<std::string, S> map_type;
    typedef std::vector<S>           vec_type;
    map_type map;
    vec_type vec;
    inline void insert(const std::string& name,
                       const S&           elem) {
      map[name] = elem;
      vec.push_back(elem);
    }
    void find(S& elem, const std::string& name, bool recurse) {
      typename map_type::iterator it = map.find(name);
      if (it != map.end()) elem = it->second;
      if (recurse && parent) {
        parent->find(elem, name, recurse);
      }
    }
  private:
    container<S>* parent;
  };

  /** Containers for each kind of symbol */
  container<Symbol*>      symbolContainer;
  container<FuncSymbol*>  funcContainer;
  container<ClassSymbol*> classContainer;

};

}
}

#endif /* VENOM_ANALYSIS_SYMBOLTABLE_H */
