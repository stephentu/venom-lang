#ifndef VENOM_ANALYSIS_SEMANTICCONTEXT_H
#define VENOM_ANALYSIS_SEMANTICCONTEXT_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <analysis/type.h>
#include <analysis/symboltable.h>
#include <util/stl.h>

namespace venom {

// TODO: clean up these forward declarations

namespace analysis {
  /** Forward decl for NewBootstrapSymbolTable forward decl */
  class SemanticContext;
}

namespace ast {
  /** Forward decl for SemanticContext::instantiateOrThrow */
  struct ParameterizedTypeString;
}

namespace bootstrap {
  /** Forward decl for SemanticContext friend */
  analysis::SymbolTable* NewBootstrapSymbolTable(analysis::SemanticContext*);
}

namespace analysis {

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
public:
  SemanticContext(const std::string& moduleName)
    : moduleName(moduleName), rootSymbols(NULL) {}

  ~SemanticContext() {
    // we have ownership of the types
    util::delete_pointers(types.begin(), types.end());
    if (rootSymbols) delete rootSymbols;
    Type::ResetBuiltinTypes();
  }

  inline std::string& getModuleName() { return moduleName; }
  inline const std::string& getModuleName() const { return moduleName; }

  inline SymbolTable* getRootSymbolTable() { return rootSymbols; }
  inline const SymbolTable* getRootSymbolTable() const { return rootSymbols; }

  /** Creation of types. Should only be called when NEW types are encountered **/
  Type* createType(const std::string& name,
                   InstantiatedType*  parent,
                   size_t             params = 0);

  Type* createTypeParam(const std::string& name, size_t pos);

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
    this->rootSymbols = rootSymbols;
  }
private:
  /** Fully qualified module name */
  std::string moduleName;

  /** All types created during semantic analysis, for memory management */
  std::vector<Type*> types;

  /** All instantiated types created during semantic analysis,
   * for memory management */
  std::vector<InstantiatedType*> itypes;

  /** Root symbol table */
  SymbolTable* rootSymbols;
};

}
}

#endif /* VENOM_ANALYSIS_SEMANTICCONTEXT_H */
