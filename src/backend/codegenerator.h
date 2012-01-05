#ifndef VENOM_BACKEND_CODE_GENERATOR_H
#define VENOM_BACKEND_CODE_GENERATOR_H

#include <cassert>
#include <map>
#include <string>
#include <vector>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <backend/symbolicbytecode.h>

#include <util/container.h>
#include <util/stl.h>

namespace venom {
namespace backend {

/** Forward decl */
class ObjectCode;

class Label {
  friend class CodeGenerator;
protected:
  Label(int64_t index) : index(index) {}
public:
  inline int64_t getIndex() const { return index; }
  inline int64_t calcOffset(const Label& that) const {
    return index - that.index;
  }
protected:
  int64_t index;
};

class SymbolReference {
public:
  SymbolReference(size_t local_index) :
    local(true), local_index(local_index) {}
  SymbolReference(const std::string& full_name) :
    local(false), full_name(full_name) {}

  inline bool isLocal() const { return local; }
  inline size_t getLocalIndex() const {
    assert(isLocal()); return local_index;
  }
  inline std::string& getFullName() { return full_name; }
  inline const std::string& getFullName() const { return full_name; }
private:
  bool local;
  size_t local_index;
  std::string full_name;
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

  /** Returns a new label which points to the current place in
   * the instruction stream */
  Label* newLabel();

  /** Returns an index used to reference into the local variable table */
  size_t createLocalVariable(analysis::Symbol* symbol);

  /** throws error if local variable does not exist */
  size_t getLocalVariable(analysis::Symbol* symbol);

  /** Returns an index used to reference into the constant pool */
  size_t createConstant(const std::string& constant);

  /** Enter local ClassSymbol into the class pool, and return an
   * index used to reference this class symbolically. The reference
   * returned is *NOT* the position in the local class pool, but rather
   * the position in the class reference table (which points into the local
   * class pool) */
  size_t enterLocalClass(analysis::ClassSymbol* symbol);

  size_t enterExternalClass(analysis::ClassSymbol* symbol);

  size_t enterClass(analysis::ClassSymbol* symbol);

  /** Enter local FuncSymbol into the function pool, and return an
   * index used to reference this function symbolically. The reference
   * returned is *NOT* the position in the local function pool, but rather
   * the position in the function reference table (which points into the local
   * function pool) */
  size_t enterLocalFunction(analysis::FuncSymbol* symbol);

  size_t enterExternalFunction(analysis::FuncSymbol* symbol);

  size_t enterFunction(analysis::FuncSymbol* symbol);

  /** Instruction emitting methods */

  void emitInst(Instruction::Opcode opcode);

  void emitInstU32(Instruction::Opcode opcode, uint32_t n0);

  //void emitInstI32(Instruction::Opcode opcode, int32_t n0);

  void emitInstLabel(Instruction::Opcode opcode, Label* label);

  void emitInstI64(Instruction::Opcode opcode, int64_t n0);

  void emitInstDouble(Instruction::Opcode opcode, double n0);

  void emitInstBool(Instruction::Opcode opcode, bool n0);

  /**
   * Create object code representation.
   * Note this can only be called *once*, and doing so gives ownership
   * of memory to ObjectCode
   */
  ObjectCode* createObjectCode();

  /** Debug helpers */
  void printDebugStream();

private:
  /** The context which this instance is generating code for */
  analysis::SemanticContext* ctx;

  /** Created labels, for memory management */
  std::vector<Label*> labels;

  /** Instruction stream */
  std::vector<SymbolicInstruction*> instructions;

  /** Does this CodeGenerator own the labels/instructions? */
  bool ownership;

  template <typename SearchType>
  struct container_table_local_functor {
    container_table_local_functor(util::container_pool<SearchType>* pool)
      : pool(pool) {}
    inline SymbolReference operator()(const SearchType& t) {
      return SymbolReference(pool->create(t));
    }
    util::container_pool<SearchType>* pool;
  };

  template <typename SearchType>
  struct container_table_external_functor {
    SymbolReference operator()(const SearchType& t) {
      return SymbolReference(t->getFullName());
    }
  };

  template <typename SearchType>
  struct container_table :
  public util::container_base<SearchType, SymbolReference> {
    container_table(util::container_pool<SearchType>* pool): pool(pool) {}
    inline size_t createLocal(const SearchType& t) {
      return createImpl(t, container_table_local_functor<SearchType>(pool));
    }
    inline size_t createExternal(const SearchType& t) {
      return createImpl(t, container_table_external_functor<SearchType>());
    }
    util::container_pool<SearchType>* pool;
  };

  /** Local variables */
  util::container_pool<analysis::Symbol*> local_variable_pool;

  /** Constant pool */
  util::container_pool<std::string> constant_pool;

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
