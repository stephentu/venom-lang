#ifndef VENOM_BACKEND_CODE_GENERATOR_H
#define VENOM_BACKEND_CODE_GENERATOR_H

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/symbolicbytecode.h>

#include <util/container.h>
#include <util/either.h>
#include <util/stl.h>

namespace venom {

namespace runtime {
  /** Forward decl */
  class venom_class_object;
}

namespace backend {

/** Forward decl */
class ObjectCode;
class FunctionDescriptor;

class Label {
  friend class CodeGenerator;
protected:
  Label() : index(-1) {}
  Label(int64_t index) : index(index) {}
public:
  inline int64_t getIndex() const { return index; }
  inline int64_t calcOffset(size_t that) const {
    return index - that;
  }
  inline bool isBound() const { return index >= 0; }
protected:
  int64_t index;
};

inline std::ostream& operator<<(std::ostream& o, const Label& l) {
  o << "label_" << l.getIndex();
  return o;
}

namespace {
  typedef util::either<size_t, std::string>::comparable _symref_parent;
}
class SymbolReference : public _symref_parent {
public:
  SymbolReference(size_t local_index) :
    _symref_parent(local_index) {}
  SymbolReference(const std::string& full_name) :
    _symref_parent(full_name) {}

  inline bool isLocal() const { return isLeft(); }
  inline bool isExternal() const { return isRight(); }

  inline size_t getLocalIndex() const { return left(); }

  inline std::string& getFullName() { return right(); }
  inline const std::string& getFullName() const { return right(); }
};

inline std::ostream& operator<<(std::ostream& o, const SymbolReference& sref) {
  if (sref.isLocal()) {
    o << ".local " << sref.getLocalIndex();
  } else {
    o << ".extern " << sref.getFullName();
  }
  return o;
}

namespace {
  typedef util::either<std::string, size_t>::comparable _constant_parent;
}
class Constant : public _constant_parent {
public:
  Constant(const std::string& data) : _constant_parent(data) {}
  Constant(size_t classIdx) : _constant_parent(classIdx) {}

  inline bool isString() const { return isLeft(); }
  inline bool isClassSingleton() const { return isRight(); }

  inline std::string& getData() { return left(); }
  inline const std::string& getData() const { return left(); }

  inline size_t getClassIdx() const { return right(); }
};

inline std::ostream& operator<<(std::ostream& o, const Constant& konst) {
  o << ".const ";
  // TODO: escape string
  if (konst.isString()) o << ".string \"" << konst.getData() << "\"";
  else o << ".classref " << konst.getClassIdx();
  return o;
}

class FunctionSignature {
public:
  // regular constructor
  FunctionSignature(
      const std::string& name,
      const std::vector<uint32_t>& parameters,
      uint32_t returnType,
      uint64_t codeOffset) :
    name(name), parameters(parameters),
    returnType(returnType), codeOffset(codeOffset) {}

  // method constructor
  FunctionSignature(
      const std::string& className,
      const std::string& name,
      const std::vector<uint32_t>& parameters,
      uint32_t returnType,
      uint64_t codeOffset) :
    className(className), name(name), parameters(parameters),
    returnType(returnType), codeOffset(codeOffset) {}

  inline bool isMethod() const { return !className.empty(); }

  inline std::string getFullName(const std::string& moduleName) const {
    return isMethod() ?
      moduleName + "." + className + "." + name :
      moduleName + "." + name;
  }

  FunctionDescriptor* createFuncDescriptor(uint64_t globalOffset);

  std::string className;
  std::string name;
  std::vector<uint32_t> parameters;
  uint32_t returnType;
  uint64_t codeOffset;
};

class ClassSignature {
public:
  // the primitives have special indicies
  static const uint32_t IntIndex   = 0xFFFFFFFFu;
  static const uint32_t FloatIndex = IntIndex - 1u;
  static const uint32_t BoolIndex  = FloatIndex - 1u;
  static const uint32_t VoidIndex  = BoolIndex - 1u;

  static inline bool IsSpecialIndex(uint32_t idx) {
    return idx == IntIndex   ||
           idx == FloatIndex ||
           idx == BoolIndex  ||
           idx == VoidIndex;
  }

  ClassSignature(
      const std::string& name,
      const std::vector<uint32_t>& attributes,
      int32_t ctor,
      const std::vector<uint32_t>& methods)
    : name(name), attributes(attributes), ctor(ctor), methods(methods) {
    assert(ctor == -1 || ctor >= 0);
  }

  runtime::venom_class_object* createClassObject(
      const std::vector<FunctionDescriptor*>& referenceTable);

  inline std::string getFullName(const std::string& moduleName) const {
    return moduleName + "." + name;
  }

  std::string name;
  std::vector<uint32_t> attributes;
  int32_t ctor;
  std::vector<uint32_t> methods;
};

/**
 * A CodeGenerator is the main data structure for emitting venom bytecode.
 *
 * A CodeGenerator is unique to a module, and ultimately produces one bytecode
 * file corresponding to one input source file.
 */
class CodeGenerator {
public:
  /** does not take ownership of ctx */
  CodeGenerator(analysis::SemanticContext* ctx) :
    ctx(ctx),
    ownership(true),
    class_reference_table(&class_pool),
    func_reference_table(&func_pool) {}

  ~CodeGenerator() {
    if (ownership) {
      util::delete_pointers(labels.begin(), labels.end());
      util::delete_pointers(instructions.begin(), instructions.end());
    }
  }

  /** Returns a new, unbound label */
  Label* newLabel();

  void bindLabel(Label *label);

  inline Label* newBoundLabel() {
    Label *label = newLabel();
    bindLabel(label);
    return label;
  }

  /** Returns an index used to reference into the local variable table */
  size_t createLocalVariable(analysis::Symbol* symbol, bool& create);

  inline void resetLocalVariables() {
    util::delete_pointers(temporary_symbols.begin(), temporary_symbols.end());
    temporary_symbols.clear();
    available_temporary_symbols.clear();
    local_variable_pool.reset();
  }

  /** Symbol returned has no symbol table and no type */
  analysis::Symbol* createTemporaryVariable();

  /** symbol must have come from createTemporaryVariable() */
  void returnTemporaryVariable(analysis::Symbol* symbol);

  /** Returns an index used to reference into the constant pool */
  size_t createConstant(const Constant& constant, bool& create);

  /** Enter local InstantiatedType into the class pool, and return an
   * index used to reference this class symbolically. The reference
   * returned is *NOT* the position in the local class pool, but rather
   * the position in the class reference table (which points into the local
   * class pool) */
  size_t enterLocalClass(analysis::InstantiatedType* klass, bool& create);

  size_t enterExternalClass(analysis::InstantiatedType* klass, bool& create);

  size_t enterClass(analysis::InstantiatedType* klass, bool& create);

  /** Enter local FuncSymbol into the function pool, and return an
   * index used to reference this function symbolically. The reference
   * returned is *NOT* the position in the local function pool, but rather
   * the position in the function reference table (which points into the local
   * function pool) */
  size_t enterLocalFunction(analysis::FuncSymbol* symbol, bool& create);

  size_t enterExternalFunction(analysis::FuncSymbol* symbol, bool& create);

  size_t enterFunction(analysis::FuncSymbol* symbol, bool& create);

  /** Instruction emitting methods */

  void emitInst(Instruction::Opcode opcode);

  void emitInstU32(Instruction::Opcode opcode, uint32_t n0);

  //void emitInstI32(Instruction::Opcode opcode, int32_t n0);

  void emitInstLabel(Instruction::Opcode opcode, Label* label);

  void emitInstI64(Instruction::Opcode opcode, int64_t n0);

  void emitInstDouble(Instruction::Opcode opcode, double n0);

  void emitInstBool(Instruction::Opcode opcode, bool n0);

  void createObjectCodeAndSet(analysis::SemanticContext* ctx);

  /** Debug helpers */
  void printDebugStream();

private:
  size_t enterLocalClass(analysis::ClassSymbol* symbol, bool& create);

  size_t enterExternalClass(analysis::ClassSymbol* symbol, bool& create);

  /** Helper for enterClass() */
  analysis::ClassSymbol* resolveToSymbol(analysis::InstantiatedType* klass);

  /**
   * Create object code representation.
   * Note this can only be called *once*, and doing so gives ownership
   * of memory to ObjectCode
   */
  ObjectCode* createObjectCode();

  /** helper for createObjectCode() */
  size_t getClassRefIndexFromType(analysis::InstantiatedType* type);

  /** helper for createClass() */
  bool isLocalSymbol(const analysis::BaseSymbol* symbol) const;

  /** The context which this instance is generating code for */
  analysis::SemanticContext* ctx;

  /** Created labels, for memory management */
  std::vector<Label*> labels;

  /** Map instruction to label(s) */
  typedef std::map<size_t, Label*> InstLabelMap;
  InstLabelMap instToLabels;

  /** Map function inst stream to label */
  typedef std::map<size_t, std::pair<Label*, analysis::BaseSymbol*> >
    InstLabelSymbolPairMap;
  InstLabelSymbolPairMap instToFuncLabels;

  /** Map function pool ref number to label */
  typedef std::map<size_t, Label*> FuncIdxLabelMap;
  FuncIdxLabelMap funcIdxToLabels;

  /** Instruction stream */
  std::vector<SymbolicInstruction*> instructions;

  /** Does this CodeGenerator own the labels/instructions? */
  bool ownership;

  template <typename SearchType>
  struct container_table_local_functor {
    container_table_local_functor(util::container_pool<SearchType>* pool)
      : pool(pool) {}
    inline SymbolReference operator()(const SearchType& t) {
      bool create;
      return SymbolReference(pool->create(t, create));
    }
    util::container_pool<SearchType>* pool;
  };

  template <typename SearchType>
  struct container_table_external_functor {
    inline SymbolReference operator()(const SearchType& t) {
      return SymbolReference(t->getFullName());
    }
  };

  template <typename SearchType>
  struct container_table :
  public util::container_base<SearchType, SymbolReference> {
    container_table(util::container_pool<SearchType>* pool): pool(pool) {}
    inline size_t createLocal(const SearchType& t, bool& create) {
      return createImpl(
          t, container_table_local_functor<SearchType>(pool), create);
    }
    inline size_t createExternal(const SearchType& t, bool& create) {
      return createImpl(
          t, container_table_external_functor<SearchType>(), create);
    }
    util::container_pool<SearchType>* pool;
  };

  /** Local variables */
  util::container_pool<analysis::Symbol*> local_variable_pool;

  /** all created temporary symbols
   * (including those on loan and those available) */
  std::set<analysis::Symbol*> temporary_symbols;

  /** currently available temporary symbols */
  std::set<analysis::Symbol*> available_temporary_symbols;

  /** Constant pool */
  util::container_pool<Constant> constant_pool;

  /** Class pool - this is where the classes defined in this module
   * are organized */
  util::container_pool<analysis::ClassSymbol*> class_pool;

  /** Class reference table */
  container_table<analysis::ClassSymbol*> class_reference_table;

  /** Function pool */
  util::container_pool<analysis::FuncSymbol*> func_pool;

  /** Function reference table */
  container_table<analysis::FuncSymbol*> func_reference_table;
};

}
}

#endif /* VENOM_BACKEND_CODE_GENERATOR_H */
