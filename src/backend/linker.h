#ifndef VENOM_BACKEND_LINKER_H
#define VENOM_BACKEND_LINKER_H

#include <string>
#include <vector>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>
#include <backend/symbolicbytecode.h>

#include <runtime/venomobject.h>

#include <util/either.h>
#include <util/container.h>
#include <util/stl.h>

namespace venom {
namespace backend {

/** Forward decl */
class FunctionDescriptor;

// TODO: labels are going to have to go, when we want to
// serialize an ObjectCode
class ObjectCode {
public:
  typedef std::vector<Constant> ConstPool;
  typedef std::vector<SymbolicInstruction*> IStream;
  typedef std::vector<Label*> LabelVec;
  typedef std::vector<SymbolReference> RefTable;
  typedef std::vector<FunctionSignature> FuncSigPool;
  typedef std::vector<ClassSignature> ClassSigPool;
  typedef std::map<std::string, size_t> NameOffsetMap;

  /** takes ownership of instructions + labels */
  ObjectCode(const std::string& moduleName,
             const ConstPool& constant_pool,
             const ClassSigPool& class_pool,
             const RefTable& class_reference_table,
             const FuncSigPool& func_pool,
             const RefTable& func_reference_table,
             const IStream& instructions,
             const NameOffsetMap& nameOffsetMap,
             const LabelVec& labels) :
    moduleName(moduleName),
    constant_pool(constant_pool),
    class_pool(class_pool),
    class_reference_table(class_reference_table),
    func_pool(func_pool),
    func_reference_table(func_reference_table),
    instructions(instructions),
    nameOffsetMap(nameOffsetMap),
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

  inline NameOffsetMap& getNameOffsetMap() { return nameOffsetMap; }
  inline const NameOffsetMap& getNameOffsetMap() const { return nameOffsetMap; }

private:
  std::string moduleName;

  ConstPool constant_pool;

  ClassSigPool class_pool;
  RefTable class_reference_table;

  FuncSigPool func_pool;
  RefTable func_reference_table;

  IStream instructions;
  NameOffsetMap nameOffsetMap;

  LabelVec labels;
};

namespace {
  typedef util::either<std::string, runtime::venom_class_object*>::comparable
          _exec_constant_parent;
}

class ExecConstant : public _exec_constant_parent {
public:
  ExecConstant(const std::string& data) :
    _exec_constant_parent(data) {}
  ExecConstant(runtime::venom_class_object* singletonClass) :
    _exec_constant_parent(singletonClass) {}
};

class Executable {
  friend class ExecutionContext;
  friend class FunctionDescriptor;
  friend class Instruction;
public:
  typedef std::vector<ExecConstant> ConstPool;
  typedef std::vector<FunctionDescriptor*> FuncDescVec;
  typedef std::vector<runtime::venom_class_object*> ClassObjVec;

  typedef util::SizedArray<Instruction*> IStream;

  Executable( /** Args for execution - takes ownership of instructions */
             const ConstPool& constant_pool,
             const IStream& instructions,
             uint64_t mainOffset,

             /* Args for mem mgnt- takes ownership of these pointers */
             const FuncDescVec& user_func_descs,
             const ClassObjVec& user_class_objs) :
    constant_pool(constant_pool),
    instructions(instructions),
    mainOffset(mainOffset),
    user_func_descs(user_func_descs),
    user_class_objs(user_class_objs) {
    assert(mainOffset < instructions.size());
  }

  ~Executable();

  inline Instruction** startingInst() {
    return instructions.begin() + mainOffset;
  }

protected:
  /** un-initialized constant pool (only holds the data) */
  ConstPool constant_pool;

  IStream instructions;
  uint64_t mainOffset; // where is <main> located, offset in the istream

  FuncDescVec user_func_descs;
  ClassObjVec user_class_objs;
};

class LinkerException : public std::runtime_error {
public:
  explicit LinkerException(const std::string& what)
    : std::runtime_error(what) {}
};

class Linker {
  friend class Executable;
  friend class ObjectCode;
public:
  typedef std::vector<ObjectCode*> ObjCodeVec;
  typedef std::vector<size_t> MapTbl;
  typedef std::map<std::string, FunctionDescriptor*> FuncDescMap;
  typedef std::map<std::string, runtime::venom_class_object*> ClassObjMap;

  Linker(const FuncDescMap& builtin_function_map,
         const ClassObjMap& builtin_class_map) :
    builtin_function_map(builtin_function_map),
    builtin_class_map(builtin_class_map) {}

  /** objs must be non-empty. objs[0] is assumed to be
   * the main module */
  Executable* link(const ObjCodeVec& objs, size_t mainIdx);

private:
  FuncDescMap builtin_function_map;
  ClassObjMap builtin_class_map;
};

}
}

#endif /* VENOM_BACKEND_LINKER_H */
