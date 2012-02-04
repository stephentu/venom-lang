/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VENOM_ANALYSIS_SYMBOLTABLE_H
#define VENOM_ANALYSIS_SYMBOLTABLE_H

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include <ast/node.h>

#include <analysis/symbol.h>
#include <analysis/typetranslator.h>

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

typedef std::vector< std::pair<InstantiatedType*, InstantiatedType*> >
        TypeMap;

class SymbolTable {
  friend class ast::ClassDeclNode;
private:
  /** Create a child symbol table */
  SymbolTable(SemanticContext* ctx, SymbolTable* primaryParent,
              ast::ASTNode* owner, ast::ASTNode* node);

  inline void addClassParent(InstantiatedType* parent,
                             const TypeMap& typeMap) {
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

    // assert our parent vec is consistent w/ the containers
    assert(symbolContainer.parents.size() == (1 + classParents.size()));

    // finally, add the class parent

    classParents.push_back(parent);

    SymbolTable* psym = parent->getClassSymbolTable();

    symbolContainer.parents.push_back(&psym->symbolContainer);
    symbolContainer.maps.push_back(typeMap);

    funcContainer.parents.push_back(&psym->funcContainer);
    funcContainer.maps.push_back(typeMap);

    classContainer.parents.push_back(&psym->classContainer);
    classContainer.maps.push_back(typeMap);
  }

public:
  /** Create the root symbol table. Does *NOT* take ownership of ctx */
  SymbolTable(SemanticContext *ctx)
    : ctx(ctx), owner(NULL), node(NULL), primaryParent(NULL) {}

  ~SymbolTable() {
    // delete children
    util::delete_pointers(children.begin(), children.end());
  }

  inline SemanticContext* getSemanticContext() { return ctx; }
  inline const SemanticContext* getSemanticContext() const { return ctx; }

  inline ast::ASTNode* getOwner() { return owner; }
  inline const ast::ASTNode* getOwner() const { return owner; }

  inline ast::ASTNode* getNode() { return node; }
  inline const ast::ASTNode* getNode() const { return node; }

  inline SymbolTable* getPrimaryParent() { return primaryParent; }
  inline const SymbolTable* getPrimaryParent() const { return primaryParent; }

  inline const std::vector<InstantiatedType*>& getClassParents() const
    { return classParents; }

  inline SymbolTable* newChildScope(SemanticContext* ctx) {
    return newChildScope(ctx, NULL, NULL);
  }

  inline SymbolTable* newChildScope(ast::ASTNode* owner, ast::ASTNode* node) {
    // the one case which is not allowed, is owner != NULL with node == NULL
    assert(!owner || node);
    return newChildScope(ctx, owner, node);
  }

  /** For builtins to use */
  inline SymbolTable* newChildScopeNoNode() {
    return newChildScope(NULL, NULL);
  }

private:
  inline SymbolTable* newChildScope(SemanticContext* ctx,
                                    ast::ASTNode* owner,
                                    ast::ASTNode* node) {
    SymbolTable *child = new SymbolTable(ctx, this, owner, node);
    children.push_back(child);
    return child;
  }

public:
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

  bool isModuleLevelSymbolTable() const;

  bool isDefined(const std::string& name, unsigned int type, RecurseMode mode);

  BaseSymbol*
  findBaseSymbol(const std::string& name, unsigned int type, RecurseMode mode,
                 TypeTranslator& translator);

  Symbol*
  createSymbol(const std::string& name, InstantiatedType* type,
               ast::ASTNode* decl);

  Symbol*
  createClassAttributeSymbol(const std::string& name,
                             InstantiatedType* type,
                             ClassSymbol* classSymbol);

  Symbol*
  findSymbol(const std::string& name, RecurseMode mode,
             TypeTranslator& translator);

  void getSymbols(std::vector<Symbol*>& symbols);

  FuncSymbol*
  createFuncSymbol(const std::string&                    name,
                   SymbolTable*                          funcTable,
                   const std::vector<InstantiatedType*>& typeParams,
                   const std::vector<InstantiatedType*>& params,
                   InstantiatedType*                     returnType,
                   bool                                  native = false);

  FuncSymbol*
  createMethodSymbol(const std::string&                    name,
                     SymbolTable*                          funcTable,
                     const std::vector<InstantiatedType*>& typeParams,
                     const std::vector<InstantiatedType*>& params,
                     InstantiatedType*                     returnType,
                     ClassSymbol*                          classSymbol,
                     bool                                  overrides = false,
                     bool                                  native = false);

  FuncSymbol*
  findFuncSymbol(const std::string& name, RecurseMode mode,
                 TypeTranslator& translator);

  void getFuncSymbols(std::vector<FuncSymbol*>& symbols);

  ClassSymbol*
  createClassSymbol(const std::string& name,
                    SymbolTable*       classTable,
                    Type*              type,
                    const std::vector<InstantiatedType*>& typeParams =
                      std::vector<InstantiatedType*>());

  ClassSymbol*
  createSpecializedClassSymbol(SymbolTable* classTable,
                               InstantiatedType* instantiation,
                               Type* type);
private:
  ClassSymbol* insertClassSymbol(ClassSymbol* symbol);

public:
  ClassSymbol*
  findClassSymbol(const std::string& name, RecurseMode mode,
                  TypeTranslator& translator);

  void getClassSymbols(std::vector<ClassSymbol*>& symbols);

  ModuleSymbol*
  findModuleSymbol(const std::string& name, RecurseMode mode);

  ModuleSymbol*
  createModuleSymbol(const std::string& name, SymbolTable* moduleTable,
                     ClassSymbol* moduleClass, SemanticContext* origCtx);

  void getModuleSymbols(std::vector<ModuleSymbol*>& symbols);

  void linearizedClassOrder(std::vector<SymbolTable*>& tables);

private:
  /** The module (context) this symbol table belongs to */
  SemanticContext*          ctx;

  /** The AST node which "owns" this scope. Can be NULL if none */
  ast::ASTNode*             owner;

  /** The AST node which defines this scope.
   * Is the child of owner (if owner is not NULL) */
  ast::ASTNode*             node;

  /** WARNING: while a SymbolTable can have multiple parents, only
   * one of its parents can have it as a child. This allows us to
   * prevent double-deletes when calling the destructor */
  SymbolTable* primaryParent;
  std::vector<InstantiatedType*> classParents;
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

  // S is expected to be a pointer type
  template <typename S>
  class container {
  public:
    container() {}
    container(const std::vector<container<S>*>& parents,
              const std::vector<TypeMap>&       maps)
      : parents(parents), maps(maps) {
      assert(parents.size() == maps.size());
    }

    ~container() {
      util::delete_pointers(vec.begin(), vec.end());
    }

    typedef std::map<std::string, S> map_type;
    typedef std::vector<S>           vec_type;
    map_type map;
    vec_type vec;

    inline void insert(const std::string& name, const S& elem) {
      std::pair< typename map_type::iterator, bool > res =
        map.insert(typename map_type::value_type(name, elem));
      assert(res.second); // new elem should always be inserted
      vec.push_back(elem);
      assert(map.size() == vec.size());
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

    // only gets in current scope (non recursive)
    void getAll(std::vector<S>& elems) {
      elems.reserve(elems.size() + vec.size());
      elems.insert(elems.end(), vec.begin(), vec.end());
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
