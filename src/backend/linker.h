#ifndef VENOM_BACKEND_LINKER_H
#define VENOM_BACKEND_LINKER_H

#include <string>
#include <vector>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>
#include <backend/symbolicbytecode.h>

#include <runtime/venomobject.h>

#include <util/container.h>
#include <util/stl.h>

namespace venom {
namespace backend {

/** Forward decl */
class FunctionDescriptor;

class ObjectCode {
public:
  typedef std::vector<Constant> ConstPool;
  typedef std::vector<SymbolicInstruction*> IStream;
  typedef std::vector<Label*> LabelVec;
  typedef std::vector<SymbolReference> RefTable;
  typedef std::vector<FunctionSignature> FuncSigPool;
  typedef std::vector<ClassSignature> ClassSigPool;

  /** takes ownership of instructions + labels */
  ObjectCode(const std::string& moduleName,
             const ConstPool& constant_pool,
             const ClassSigPool& class_pool,
             const RefTable& class_reference_table,
             const FuncSigPool& func_pool,
             const RefTable& func_reference_table,
             const IStream& instructions,
             const LabelVec& labels) :
    moduleName(moduleName)
    constant_pool(constant_pool),
    class_pool(class_pool),
    class_reference_table(class_reference_table),
    func_pool(func_pool),
    func_reference_table(func_reference_table),
    instructions(instructions),
    labels(labels) {}

  ~ObjectCode() {
    util::delete_pointers(labels.begin(), labels.end());
    util::delete_pointers(instructions.begin(), instructions.end());
  }

  /** Fully qualified module name */
  inline std::string& getModuleName() { return moduleName; }
  inline const std::string& getModuleName() const { return moduleName; }

  inline ConstPool& getConstantPool() { return constant_pool; }
  inline const ConstPool& getConstantPool() const { return constant_pool; }

  inline ClassSigPool& getClassPool() { return class_pool; }
  inline const ClassSigPool& getClassPool() const { return class_pool; }

  inline RefTable& getClassRefTable() { return class_reference_table; }
  inline const RefTable& getClassRefTable() const { return class_reference_table; }

  inline FuncSigPool& getFuncPool() { return func_pool; }
  inline const FuncSigPool& getFuncPool() const { return func_pool; }

  inline RefTable& getFuncRefTable() { return func_reference_table; }
  inline const RefTable& getFuncRefTable() const { return func_reference_table; }

  inline IStream& getInstructions() { return instructions; }
  inline const IStream& getInstructions() const { return instructions; }

private:
  std::string moduleName;

  ConstPool constant_pool;

  ClassSigPool class_pool;
  RefTable class_reference_table;

  FuncSigPool func_pool;
  RefTable func_reference_table;

  IStream instructions;
  LabelVec labels;
};

class Executable {
  friend class ExecutionContext;
public:
  typedef std::vector<Constant> ConstPool;
  typedef util::SizedArray<Instruction*> IStream;
  typedef util::SizedArray<runtime::venom_class_object*> ClassObjPool;

  /** has ownership of class_obj_pool and instructions */
  Executable(const ConstPool& constant_pool,
             const ClassObjPool& class_obj_pool,
             const IStream& instructions) :
    constant_pool(constant_pool),
    class_obj_pool(class_obj_pool),
    instructions(instructions) {}

  ~Executable() {
    //util::delete_pointers(class_obj_pool.begin(), class_obj_pool.end());
    util::delete_pointers(instructions.begin(), instructions.end());
    delete [] instructions.begin();
  }

protected:
  /** un-initialized constant pool (only holds the data) */
  ConstPool constant_pool;
  ClassObjPool class_obj_pool;
  IStream instructions;
};

class LinkerException : public std::runtime_error {
public:
  explicit LinkerException(const std::string& what)
    : std::runtime_error(what) {}
};

class Linker {
  friend class ObjectCode;
  friend class Executable;
public:
  typedef std::vector<ObjectCode*> ObjCodeVec;
  typedef std::vector<size_t> MapTbl;
  typedef std::map<std::string, FunctionDescriptor*> FuncDescMap;

  Linker(const FuncDescMap& builtin_function_map) :
    builtin_function_map(builtin_function_map) {}

  Executable* link(const ObjCodeVec& objs);

private:
  FuncDescMap builtin_function_map;
};

}
}

#endif /* VENOM_BACKEND_LINKER_H */
