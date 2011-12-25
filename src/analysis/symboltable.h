#ifndef VENOM_ANALYSIS_SYMBOLTABLE_H
#define VENOM_ANALYSIS_SYMBOLTABLE_H

#include <map>
#include <string>
#include <vector>

#include <ast/node.h>
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
  SymbolTable(SymbolTable* parent, ast::ASTNode* owner)
    : parent(parent),
      owner(owner),
      symbolContainer(SAFE_ADDR(parent, symbolContainer)),
      funcContainer(SAFE_ADDR(parent, funcContainer)),
      classContainer(SAFE_ADDR(parent, classContainer)) {}
public:
  /** Create the root symbol table */
  SymbolTable()
    : parent(NULL),
      owner(NULL),
      symbolContainer(NULL),
      funcContainer(NULL),
      classContainer(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

  inline SymbolTable* getParent() { return parent; }
  inline const SymbolTable* getParent() const { return parent; }

  inline ast::ASTNode* getOwner() { return owner; }
  inline const ast::ASTNode* getOwner() const { return owner; }

  inline SymbolTable* newChildScope(ast::ASTNode* owner) {
    SymbolTable *child = new SymbolTable(this, owner);
    children.push_back(child);
    return child;
  }

  enum SymType {
    Location = 0x1,
    Function = 0x1 << 1,
    Class    = 0x1 << 2,
    Any      = (unsigned)-1,
  };

  bool isDefined(const std::string& name, unsigned int type, bool recurse);

  BaseSymbol*
  findBaseSymbol(const std::string& name, unsigned int type, bool recurse);

  Symbol*
  createSymbol(const std::string& name,
               InstantiatedType*  type);

  Symbol*
  findSymbol(const std::string& name, bool recurse);

  FuncSymbol*
  createFuncSymbol(const std::string&                    name,
                   const std::vector<InstantiatedType*>& params,
                   InstantiatedType*                     returnType);

  FuncSymbol*
  findFuncSymbol(const std::string& name, bool recurse);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    SymbolTable*       classTable,
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

  ClassSymbol*
  findClassSymbolOrThrow(const ast::ParameterizedTypeString* name,
                         bool recurse);

private:
  SymbolTable*              parent;
  ast::ASTNode*             owner;
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
      if (it != map.end()) {
        elem = it->second;
        return;
      }
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
