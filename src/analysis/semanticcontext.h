#ifndef VENOM_ANALYSIS_SEMANTICCONTEXT_H
#define VENOM_ANALYSIS_SEMANTICCONTEXT_H

#include <cassert>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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

namespace backend { class ObjectCode; }

namespace bootstrap {
  analysis::SymbolTable* NewBootstrapSymbolTable(analysis::SemanticContext*);
}

void unsafe_compile(
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
  friend SymbolTable* bootstrap::NewBootstrapSymbolTable(SemanticContext*);
  friend void venom::unsafe_compile(
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
