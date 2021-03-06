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

#ifndef VENOM_ANALYSIS_SEMANTICCONTEXT_H
#define VENOM_ANALYSIS_SEMANTICCONTEXT_H

#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include <analysis/type.h>
#include <analysis/symboltable.h>
#include <util/stl.h>

namespace venom {

// TODO: clean up these forward declarations

namespace analysis { class SemanticContext; }

namespace ast {
  struct ParameterizedTypeString;
  struct ASTStatementNode;
}

namespace backend {
  class CodeGenerator;
  class ObjectCode;
}

namespace bootstrap {
  analysis::SymbolTable* NewBootstrapSymbolTable(analysis::SemanticContext*);
}

void unsafe_compile_module(
    const std::string&, std::fstream&, analysis::SemanticContext&);

namespace analysis {

/**
 * Indicates that a parse error has occured
 */
class ParseErrorException : public std::runtime_error {
public:
  explicit ParseErrorException(const std::string& what)
    : std::runtime_error(what) {}
};

/**
 * Indicates that a semantic error has occured during program
 * semantic analysis.
 */
class SemanticViolationException : public std::runtime_error {
public:
  explicit SemanticViolationException(const std::string& what)
    : std::runtime_error(what) {}
};

/**
 * Indicates that a type error occurred during type checking
 */
class TypeViolationException : public std::runtime_error {
public:
  explicit TypeViolationException(const std::string& what)
    : std::runtime_error(what) {}
};

/**
 * This class represents a module. In a module, it manages
 * all types created. It does not managed scope, however.
 * So it will return a type with a certain name if it exists
 * in the module, regardless of whether or not the scope allows it.
 * Use the symbol table to check scoping rules.
 */
class SemanticContext {
  friend class backend::CodeGenerator;
  friend SymbolTable* bootstrap::NewBootstrapSymbolTable(SemanticContext*);
  friend void venom::unsafe_compile_module(
      const std::string&, std::fstream&, SemanticContext&);
private:
  SemanticContext(const std::string& moduleName,
                  SemanticContext* parent,
                  SemanticContext* programRoot)
    : moduleName(moduleName), moduleRoot(NULL), objectCode(NULL),
      parent(parent), programRoot(programRoot), rootSymbols(NULL) {}

protected:
  /** Takes ownership */
  inline void setModuleRoot(ast::ASTStatementNode* moduleRoot) {
    assert(moduleRoot);
    assert(!this->moduleRoot);
    this->moduleRoot = moduleRoot;
  }

  /** Takes ownership */
  inline void setObjectCode(backend::ObjectCode* objectCode) {
    assert(objectCode);
    assert(!this->objectCode);
    this->objectCode = objectCode;
  }

public:
  SemanticContext(const std::string& moduleName)
    : moduleName(moduleName), moduleRoot(NULL), objectCode(NULL),
      parent(NULL), programRoot(this), rootSymbols(NULL) {}

  ~SemanticContext();

  inline std::string& getModuleName() { return moduleName; }
  inline const std::string& getModuleName() const { return moduleName; }

  std::string getFullModuleName() const;

  bool isDescendantOf(const SemanticContext* that) const;

  inline SemanticContext*
    getProgramRoot() { return programRoot; }
  inline const SemanticContext*
    getProgramRoot() const { return programRoot; }

  inline ast::ASTStatementNode*
    getModuleRoot() { return moduleRoot; }
  inline const ast::ASTStatementNode*
    getModuleRoot() const { return moduleRoot; }

  inline backend::ObjectCode*
    getObjectCode() { return objectCode; }
  inline const backend::ObjectCode*
    getObjectCode() const { return objectCode; }

  /** TODO: use atomic instructions if we need thread safety */
  inline uint64_t uniqueId() { return idGen++; }

  inline std::string tempVarName() {
    std::stringstream buf;
    buf << "_tmp$$" << uniqueId();
    return buf.str();
  }

  /** Proceeds in a depth-first manner */
  template <typename Functor>
  void forEachModule(Functor functor) {
    if (moduleRoot) functor(moduleRoot, this);
    for (std::vector<SemanticContext*>::iterator it = children.begin();
         it != children.end(); ++it) {
      (*it)->forEachModule(functor);
    }
  }

  typedef std::vector< std::pair< ast::ASTStatementNode*, SemanticContext* > >
          ModuleVec;
  struct ModuleCollectorFunctor {
    ModuleCollectorFunctor(ModuleVec* modules) : modules(modules) {}
    inline void operator()(ast::ASTStatementNode* root,
                           SemanticContext* ctx) const {
      modules->push_back(std::make_pair(root, ctx));
    }
    private:
    ModuleVec* modules;
  };
  void getAllModules(ModuleVec& modules) {
    forEachModule(ModuleCollectorFunctor(&modules));
  }

  void collectObjectCode(std::vector<backend::ObjectCode*>& objCodes);

  /** Root symbol table of the *module* */
  inline SymbolTable* getRootSymbolTable() { return rootSymbols; }
  inline const SymbolTable* getRootSymbolTable() const { return rootSymbols; }

  inline bool isRootContext() const { return !parent; }

  SemanticContext* newChildContext(const std::string& moduleName);

  SemanticContext* findModule(const util::StrVec& names);
  SemanticContext* createModule(const util::StrVec& names);

  /** Creation of types. Should only be called when NEW types are encountered **/
  Type* createType(const std::string& name,
                   InstantiatedType*  parent,
                   size_t             params = 0);

  Type* createTypeParam(const std::string& name, size_t pos);

  Type* createModuleType(const std::string& name);

  InstantiatedType*
  createInstantiatedType(Type* type,
                         const std::vector<InstantiatedType*>& params);

  /** Instantiate a type, from the scope of the symbol table */
  InstantiatedType*
  instantiateOrThrow(SymbolTable *symbols,
                     const ast::ParameterizedTypeString* type);

  struct InstantiateFunctor {
    InstantiateFunctor(SemanticContext* ctx, SymbolTable* st)
      : ctx(ctx), st(st) {}
    inline InstantiatedType* operator()(
        const ast::ParameterizedTypeString* t) const {
      return ctx->instantiateOrThrow(st, t);
    }
    private:
    SemanticContext* ctx;
    SymbolTable*     st;
  };

protected:
  /** Takes ownership of rootSymbols */
  inline void setRootSymbolTable(SymbolTable* rootSymbols) {
    assert(rootSymbols);
    assert(!this->rootSymbols);
    this->rootSymbols = rootSymbols;
  }
private:
  /** Module name */
  std::string moduleName;

  /** Root AST node for the module */
  ast::ASTStatementNode* moduleRoot;

  /** Compiled code for the module */
  backend::ObjectCode* objectCode;

  /** Parent module (NULL if root) */
  SemanticContext* parent;

  /** Root for the *program* */
  SemanticContext* programRoot;

  /** Children */
  std::map<std::string, SemanticContext*> childrenMap;
  std::vector<SemanticContext*>           children;

  /** All types created during semantic analysis, for memory management */
  std::vector<Type*> types;

  /** All instantiated types created during semantic analysis,
   * for memory management */
  std::vector<InstantiatedType*> itypes;

  /** Module root symbol table */
  SymbolTable* rootSymbols;

  uint64_t idGen;
};

}
}

#endif /* VENOM_ANALYSIS_SEMANTICCONTEXT_H */
