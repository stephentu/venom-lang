#ifndef VENOM_ANALYSIS_SYMBOLTABLE_H
#define VENOM_ANALYSIS_SYMBOLTABLE_H

#include <map>
#include <string>
#include <vector>
#include <utility>

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

/** Forward decl */
class InstantiatedType;
class SemanticContext;
typedef std::vector< std::pair<InstantiatedType*, InstantiatedType*> > TypeMap;

class TypeTranslator {
  friend class SymbolTable;
  friend class FuncSymbol;
public:
  TypeTranslator() {}
  InstantiatedType* translate(SemanticContext* ctx, InstantiatedType* type);
  void bind(InstantiatedType* obj);
protected:
  TypeMap map;
};

class SymbolTable {
  friend class ast::ClassDeclNode;
private:
  /** Create a child symbol table */
  SymbolTable(SymbolTable* parent, const TypeMap& typeMap, ast::ASTNode* owner);

protected:
  inline void addParent(SymbolTable* parent, const TypeMap& typeMap) {
    symbolContainer.parents.push_back(&parent->symbolContainer);
    symbolContainer.maps.push_back(typeMap);

    funcContainer.parents.push_back(&parent->funcContainer);
    funcContainer.maps.push_back(typeMap);

    classContainer.parents.push_back(&parent->classContainer);
    classContainer.maps.push_back(typeMap);
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
    SymbolTable *child = new SymbolTable(this, TypeMap(), owner);
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
  findBaseSymbol(const std::string& name, unsigned int type, bool recurse,
                 TypeTranslator& translator);

  Symbol*
  createSymbol(const std::string& name,
               InstantiatedType*  type);

  Symbol*
  findSymbol(const std::string& name, bool recurse,
             TypeTranslator& translator);

  FuncSymbol*
  createFuncSymbol(const std::string&                    name,
                   const std::vector<InstantiatedType*>& typeParams,
                   const std::vector<InstantiatedType*>& params,
                   InstantiatedType*                     returnType);

  FuncSymbol*
  findFuncSymbol(const std::string& name, bool recurse,
                 TypeTranslator& translator);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    SymbolTable*       classTable,
                    Type*              type,
                    const std::vector<InstantiatedType*>& typeParams =
                      std::vector<InstantiatedType*>());

  ClassSymbol*
  findClassSymbol(const std::string& name, bool recurse,
                  TypeTranslator& translator);

  /**
   * Recurse only applies to the outer type, not the parameterized types
   *
   * The parameterized types will be recusively checked regardless of
   * recurse.
   */
  //ClassSymbol*
  //findClassSymbol(const ast::ParameterizedTypeString* name,
  //                bool recurse,
  //                const ast::ParameterizedTypeString*& failed_type,
  //                bool& wrong_params);

  //ClassSymbol*
  //findClassSymbolOrThrow(const ast::ParameterizedTypeString* name,
  //                       bool recurse);

private:
  /** WARNING: while a SymbolTable can have multiple parents, only
   * one of its parents can have it as a child. This allows us to
   * prevent double-deletes when calling the destructor */
  ast::ASTNode*             owner;
  std::vector<SymbolTable*> children;

  template <typename S>
  class container {
  public:
    container() {}
    container(const std::vector<container<S>*>& parents,
              const std::vector<TypeMap>&       maps)
      : parents(parents), maps(maps) {
      assert(parents.size() == maps.size());
    }
    typedef std::map<std::string, S> map_type;
    typedef std::vector<S>           vec_type;
    map_type map;
    vec_type vec;
    inline void insert(const std::string& name,
                       const S&           elem) {
      map[name] = elem;
      vec.push_back(elem);
    }
    bool find(S& elem, const std::string& name, bool recurse,
              TypeTranslator& translator) {
      // TODO: don't allow certain lookups to cross parent
      // boundaries
      typename map_type::iterator it = map.find(name);
      if (it != map.end()) {
        elem = it->second;
        return true;
      }
      if (recurse) {
        for (size_t i = 0; i < parents.size(); i++) {
          if (parents[i]->find(elem, name, true, translator)) {
            TypeMap &tm = maps[i];
            translator.map.insert(translator.map.end(), tm.begin(), tm.end());
            return true;
          }
        }
      }
      return false;
    }
    std::vector<container<S>*> parents;
    std::vector<TypeMap>       maps;
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
