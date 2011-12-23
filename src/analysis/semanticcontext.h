#ifndef VENOM_ANALYSIS_SEMANTICCONTEXT_H
#define VENOM_ANALYSIS_SEMANTICCONTEXT_H

#include <map>
#include <stdexcept>
#include <string>

#include <analysis/type.h>
#include <analysis/symboltable.h>
#include <util/stl.h>

/** Forward decl of main, for friend function */
int main(int, char**);

namespace venom {
namespace analysis {

class SemanticViolationException : public std::runtime_error {
public:
  explicit SemanticViolationException(const std::string& what)
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
  friend int ::main(int, char**);
public:
  SemanticContext(const std::string& moduleName)
    : moduleName(moduleName), rootSymbols(NULL) {}

  ~SemanticContext() {
    // we have ownership of the types
    util::delete_pointers(types.begin(), types.end());
    if (rootSymbols) delete rootSymbols;
  }

  inline std::string& getModuleName() { return moduleName; }
  inline const std::string& getModuleName() const { return moduleName; }

  /** Type must not already exist */
  Type* createType(const std::string& name, Type* parent, size_t params = 0);

  inline SymbolTable* getRootSymbolTable() { return rootSymbols; }
  inline const SymbolTable* getRootSymbolTable() const { return rootSymbols; }

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

  /** Root symbol table */
  SymbolTable* rootSymbols;
};

}
}

#endif /* VENOM_ANALYSIS_SEMANTICCONTEXT_H */
