#ifndef VENOM_ANALYSIS_SEMANTICCONTEXT_H
#define VENOM_ANALYSIS_SEMANTICCONTEXT_H

#include <map>
#include <string>

#include <analysis/type.h>
#include <util/stl.h>

namespace venom {
namespace analysis {

/**
 * This class represents a module. In a module, it manages
 * all types created. It does not managed scope, however.
 * So it will return a type with a certain name if it exists
 * in the module, regardless of whether or not the scope allows it.
 * Use the symbol table to check scoping rules.
 */
class SemanticContext {
public:
  SemanticContext(const std::string& moduleName)
    : moduleName(moduleName) {}

  ~SemanticContext() {
    // we have ownership of the types
    util::delete_value_pointers(types.begin(), types.end());
  }

  inline std::string& getModuleName() { return moduleName; }
  inline const std::string& getModuleName() const { return moduleName; }

  /** Returns NULL if not found */
  Type* getType(const std::string& name);

  /** Type must not already exist */
  Type* createType(const std::string& name, Type* parent, size_t params = 0);

private:
  /** Fully qualified module name */
  std::string moduleName;

  std::map<std::string, Type*> types;
};

}
}

#endif /* VENOM_ANALYSIS_SEMANTICCONTEXT_H */
