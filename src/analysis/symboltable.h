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
  SymbolTable(SemanticContext* ctx, SymbolTable* parent,
              const TypeMap& typeMap, ast::ASTNode* owner);

protected:
  inline void addClassParent(SymbolTable* parent, const TypeMap& typeMap) {
    // assert we already have a primary parent
    assert(getPrimaryParent());

    // assert we aren't adding garbage
    assert(parent);

    // assert the containers already have a parent
    assert(!symbolContainer.parents.empty());
    assert(!funcContainer.parents.empty());
    assert(!classContainer.parents.empty());

    // assert the containers are consistent with themselves
    assert(symbolContainer.parents.size() == symbolContainer.maps.size());
    assert(funcContainer.parents.size() == funcContainer.maps.size());
    assert(classContainer.parents.size() == classContainer.maps.size());

    // assert the containers are consistent with each other
    assert(symbolContainer.parents.size() == funcContainer.parents.size());
    assert(funcContainer.parents.size() == classContainer.parents.size());

    // finally, add the class parent

    symbolContainer.parents.push_back(&parent->symbolContainer);
    symbolContainer.maps.push_back(typeMap);

    funcContainer.parents.push_back(&parent->funcContainer);
    funcContainer.maps.push_back(typeMap);

    classContainer.parents.push_back(&parent->classContainer);
    classContainer.maps.push_back(typeMap);
  }

public:
  /** Create the root symbol table. Does *NOT* take ownership of ctx */
  SymbolTable(SemanticContext *ctx) : ctx(ctx), owner(NULL), parent(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

  inline SemanticContext* getSemanticContext() { return ctx; }
  inline const SemanticContext* getSemanticContext() const { return ctx; }

  inline ast::ASTNode* getOwner() { return owner; }
  inline const ast::ASTNode* getOwner() const { return owner; }

  inline SymbolTable* getPrimaryParent() { return parent; }
  inline const SymbolTable* getPrimaryParent() const { return parent; }

  inline SymbolTable*
  newChildScope(SemanticContext* ctx, ast::ASTNode* owner) {
    SymbolTable *child = new SymbolTable(ctx, this, TypeMap(), owner);
    children.push_back(child);
    return child;
  }

  inline SymbolTable* newChildScope(ast::ASTNode* owner) {
    return newChildScope(ctx, owner);
  }

  enum RecurseMode {
    NoRecurse,
    AllowCurrentScope,
    DisallowCurrentScope,
    ClassLookup,
    ClassParents,
  };

  enum SymType {
    Location = 0x1,
    Function = 0x1 << 1,
    Class    = 0x1 << 2,
    Module   = 0x1 << 3,
    Any      = (unsigned)-1,
  };

  /**
   * Is this symbol table a primary child of parent?  In other words, is parent
   * a primary ancestor of this table?
   */
  bool belongsTo(const SymbolTable* parent) const;

  /**
   * Is this symbol visible from this symbol table? In other words, is the
   * symbol either defined in this table or in some parent of this table?
   */
  bool canSee(BaseSymbol *sym);

  bool isDefined(const std::string& name, unsigned int type, RecurseMode mode);

  BaseSymbol*
  findBaseSymbol(const std::string& name, unsigned int type, RecurseMode mode,
                 TypeTranslator& translator);

  Symbol*
  createSymbol(const std::string& name,
               InstantiatedType*  type);

  Symbol*
  findSymbol(const std::string& name, RecurseMode mode,
             TypeTranslator& translator);

  FuncSymbol*
  createFuncSymbol(const std::string&                    name,
                   const std::vector<InstantiatedType*>& typeParams,
                   const std::vector<InstantiatedType*>& params,
                   InstantiatedType*                     returnType,
                   bool                                  native = false);

  FuncSymbol*
  findFuncSymbol(const std::string& name, RecurseMode mode,
                 TypeTranslator& translator);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    SymbolTable*       classTable,
                    Type*              type,
                    const std::vector<InstantiatedType*>& typeParams =
                      std::vector<InstantiatedType*>());

  ClassSymbol*
  findClassSymbol(const std::string& name, RecurseMode mode,
                  TypeTranslator& translator);

  ModuleSymbol*
  findModuleSymbol(const std::string& name, RecurseMode mode);

  ModuleSymbol*
  createModuleSymbol(const std::string& name, SymbolTable* moduleTable,
                     Type* moduleType, SemanticContext* origCtx);

private:
  /** The module (context) this symbol table belongs to */
  SemanticContext*          ctx;

  /** The AST node which "owns" this scope. Can be NULL if none */
  ast::ASTNode*             owner;

  /** WARNING: while a SymbolTable can have multiple parents, only
   * one of its parents can have it as a child. This allows us to
   * prevent double-deletes when calling the destructor */
  SymbolTable*              parent;
  std::vector<SymbolTable*> children;

  static inline void AssertValidRecurseMode(RecurseMode mode) {
    assert(mode == NoRecurse ||
           mode == AllowCurrentScope ||
           mode == DisallowCurrentScope ||
           mode == ClassLookup ||
           mode == ClassParents);
  }

  template <typename T>
  struct default_find_filter {
    inline bool operator()(const T& t) const { return true; }
  };

  template <typename T>
  struct equality_find_filter {
    equality_find_filter(const T& t) : t(t) {}
    inline bool operator()(const T& v) const { return t == v; }
    T t;
  };

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

    inline void insert(const std::string& name, const S& elem) {
      map[name] = elem;
      vec.push_back(elem);
    }

    inline bool find(S& elem, const std::string& name,
                     RecurseMode mode, TypeTranslator& translator) {
      return find(elem, name, mode, translator, default_find_filter<S>());
    }

    template <typename Filter>
    inline bool find(S& elem, const std::string& name, RecurseMode mode,
                     TypeTranslator& translator, Filter filter) {
      AssertValidRecurseMode(mode);
      if (mode == ClassLookup || mode == ClassParents) {
        return find0(elem, name,
                     mode == ClassParents ?
                        DisallowCurrentScope : AllowCurrentScope,
                     translator, true, false, filter);
      }
      return find0(elem, name, mode, translator, false, false, filter);
    }

    template <typename Filter>
    bool find0(S& elem, const std::string& name,
               RecurseMode mode,
               TypeTranslator& translator,
               bool excludeFirstParent, bool isParentScope,
               Filter filter) {
      assert(mode != ClassLookup && mode != ClassParents);
      if (isParentScope || mode != DisallowCurrentScope) {
        typename map_type::iterator it = map.find(name);
        if (it != map.end() && filter(it->second)) {
          elem = it->second;
          return true;
        }
      }
      if (mode == NoRecurse) return false;
      for (size_t i = excludeFirstParent ? 1 : 0; i < parents.size(); i++) {
        if (parents[i]->find0(
              elem, name, mode, translator, i != 0, true, filter)) {
          TypeMap &tm = maps[i];
          translator.map.insert(translator.map.end(), tm.begin(), tm.end());
          return true;
        }
      }
      return false;
    }

    std::vector<container<S>*> parents;
    std::vector<TypeMap>       maps;
  };

  template <typename S>
  struct CType {
    typedef std::vector<container<S>*> vec_type;
    typedef const std::vector<container<S>*> const_vec_type;
    typedef container<S>* result_type;
  };

  /** Containers for each kind of symbol */
  // TODO: consider merging these containers together (into one
  // container which contains a BaseSymbol*). for some reason, it
  // seemed like a good idea to separate them before, but I am not so
  // sure anymore
  container<Symbol*>       symbolContainer;
  container<FuncSymbol*>   funcContainer;
  container<ClassSymbol*>  classContainer;
  container<ModuleSymbol*> moduleContainer;
};

}
}

#endif /* VENOM_ANALYSIS_SYMBOLTABLE_H */
