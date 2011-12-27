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
  class ClassDeclNode;
}

namespace analysis {

class SymbolTable {
  friend class ast::ClassDeclNode;
private:
  /** Create a child symbol table */
  SymbolTable(SymbolTable* parent, ast::ASTNode* owner);
  SymbolTable(const std::vector<SymbolTable*>& parents, ast::ASTNode* owner);

protected:
  inline void addParent(SymbolTable* parent) {
    parents.push_back(parent);
    symbolContainer.parents.push_back(&parent->symbolContainer);
    funcContainer.parents.push_back(&parent->funcContainer);
    classContainer.parents.push_back(&parent->classContainer);
  }

public:
  /** Create the root symbol table */
  SymbolTable()
    : owner(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

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
  /** WARNING: while a SymbolTable can have multiple parents, only
   * one of its parents can have it as a child. This allows us to
   * prevent double-deletes when calling the destructor */
  std::vector<SymbolTable*> parents;
  ast::ASTNode*             owner;
  std::vector<SymbolTable*> children;

  template <typename S>
  class container {
  public:
    container() {}
    container(const std::vector<container<S>*>& parents)
      : parents(parents) {}
    typedef std::map<std::string, S> map_type;
    typedef std::vector<S>           vec_type;
    map_type map;
    vec_type vec;
    inline void insert(const std::string& name,
                       const S&           elem) {
      map[name] = elem;
      vec.push_back(elem);
    }
    bool find(S& elem, const std::string& name, bool recurse) {
      typename map_type::iterator it = map.find(name);
      if (it != map.end()) {
        elem = it->second;
        return true;
      }
      if (recurse) {
        typename std::vector<container<S>*>::iterator it = parents.begin();
        for (; it != parents.end(); ++it) {
          if ((*it)->find(elem, name, true)) return true;
        }
      }
      return false;
    }
    std::vector<container<S>*> parents;
  };

  template <typename S>
  struct CType {
    typedef std::vector<container<S>*>       vec_type;
    typedef const std::vector<container<S>*> const_vec_type;
    typedef container<S>*                    result_type;
    inline container<S>* operator()(SymbolTable* symtab) const {
      VENOM_UNIMPLEMENTED;
    }
  };

  /** Containers for each kind of symbol */
  container<Symbol*>      symbolContainer;
  container<FuncSymbol*>  funcContainer;
  container<ClassSymbol*> classContainer;

};

template <> inline SymbolTable::container<Symbol*>*
SymbolTable::CType<Symbol*>::operator()(SymbolTable* symtab) const {
  return &symtab->symbolContainer;
}

template <> inline SymbolTable::container<FuncSymbol*>*
SymbolTable::CType<FuncSymbol*>::operator()(SymbolTable* symtab) const {
  return &symtab->funcContainer;
}

template <> inline SymbolTable::container<ClassSymbol*>*
SymbolTable::CType<ClassSymbol*>::operator()(SymbolTable* symtab) const {
  return &symtab->classContainer;
}

}
}

#endif /* VENOM_ANALYSIS_SYMBOLTABLE_H */
