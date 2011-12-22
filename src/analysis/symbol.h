#ifndef VENOM_ANALYSIS_SYMBOL_H
#define VENOM_ANALYSIS_SYMBOL_H

#include <string>
#include <vector>

#include <analysis/type.h>

namespace venom {
namespace analysis {

/** Forward decl */
class SymbolTable;

class BaseSymbol {
public:
  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }
protected:
  BaseSymbol(const std::string& name) : name(name) {}
  std::string name;
};

/**
 * Represents a location in memory, such as a variable, or a
 * object's field.
 */
class Symbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  Symbol(const std::string& name, InstantiatedType* type)
    : BaseSymbol(name), type(type) {}

public:

  inline InstantiatedType* getInstantiatedType() { return type; }
  inline const InstantiatedType* getInstantiatedType() const { return type; }

private:
  InstantiatedType* type;
};

/**
 * Represents a function/method declaration
 */
class FuncSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  FuncSymbol(const std::string&        name,
             const std::vector<Type*>& params,
             Type*                     returnType)
    : BaseSymbol(name), params(params), returnType(returnType) {}

public:
  inline std::vector<Type*> getParams() { return params; }
  inline const std::vector<Type*> getParams() const { return params; }

  inline Type* getReturnType() { return returnType; }
  inline const Type* getReturnType() const { return returnType; }
private:
  std::vector<Type*> params;
  Type*              returnType;
};

/**
 * Represents a class declaration
 */
class ClassSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  ClassSymbol(const std::string& name,
              Type*              type)
    : BaseSymbol(name), type(type) {}

public:
  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }
private:
  Type* type;
};

}
}

#endif /* VENOM_ANALYSIS_SYMBOL_H */
