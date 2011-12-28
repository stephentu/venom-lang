#ifndef VENOM_ANALYSIS_SYMBOL_H
#define VENOM_ANALYSIS_SYMBOL_H

#include <string>
#include <vector>

#include <analysis/type.h>

namespace venom {
namespace analysis {

/** Forward decl */
class SemanticContext;
class SymbolTable;
class TypeTranslator;

class BaseSymbol {
public:
  virtual ~BaseSymbol() {}

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline SymbolTable* getDefinedSymbolTable() { return table; }
  inline const SymbolTable* getDefinedSymbolTable() const { return table; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params) = 0;

protected:
  BaseSymbol(const std::string& name,
             SymbolTable*       table)
    : name(name), table(table) {}
  std::string  name;
  SymbolTable* table;
};

/**
 * Represents a location in memory, such as a variable, or a
 * object's field.
 */
class Symbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  Symbol(const std::string& name,
         SymbolTable*       table,
         InstantiatedType*  type)
    : BaseSymbol(name, table), type(type) {}

public:
  inline InstantiatedType* getInstantiatedType() { return type; }
  inline const InstantiatedType* getInstantiatedType() const { return type; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

private:
  InstantiatedType* type;
};

/**
 * Represents a function/method declaration
 */
class FuncSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  FuncSymbol(const std::string&                    name,
             SymbolTable*                          table,
             const std::vector<InstantiatedType*>& params,
             InstantiatedType*                     returnType)
    : BaseSymbol(name, table), params(params), returnType(returnType) {}

public:
  inline std::vector<InstantiatedType*>
    getParams() { return params; }
  inline const std::vector<InstantiatedType*>
    getParams() const { return params; }

  inline InstantiatedType*
    getReturnType() { return returnType; }
  inline const InstantiatedType*
    getReturnType() const { return returnType; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

  bool isConstructor() const;

  bool isMethod() const;

private:
  std::vector<InstantiatedType*> params;
  InstantiatedType*              returnType;
};

/**
 * Represents a class declaration
 */
class ClassSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  ClassSymbol(const std::string& name,
              SymbolTable*       table,      /* defined */
              SymbolTable*       classTable, /* class's table */
              Type*              type)
    : BaseSymbol(name, table), classTable(classTable), type(type) {}

public:
  inline SymbolTable* getClassSymbolTable() { return classTable; }
  inline const SymbolTable* getClassSymbolTable() const { return classTable; }

  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

private:
  SymbolTable* classTable;
  Type*        type;
};

}
}

#endif /* VENOM_ANALYSIS_SYMBOL_H */
