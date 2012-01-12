#include <stdexcept>

#include <ast/statement/node.h>

#include <backend/codegenerator.h>
#include <backend/linker.h>
#include <backend/vm.h>

#include <runtime/venomobject.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::runtime;

namespace venom {
namespace backend {

FunctionDescriptor*
FunctionSignature::createFuncDescriptor(uint64_t globalOffset) {
  // construct the argument ref count bitmap
  uint64_t arg_ref_cell_bitmap = 0;
  for (size_t i = 0; i < parameters.size(); i++) {
    if (!ClassSignature::IsSpecialIndex(parameters[i])) {
      arg_ref_cell_bitmap |= (0x1UL << i);
    }
  }

  if (isMethod()) {
    // make room for the self object
    arg_ref_cell_bitmap <<= 1;
    arg_ref_cell_bitmap |= 0x1;
  }

  return new FunctionDescriptor(
      (void*) (codeOffset + globalOffset),
      isMethod() ? parameters.size() + 1 : parameters.size(),
      arg_ref_cell_bitmap,
      false);
}

venom_class_object*
ClassSignature::createClassObject(
    const vector<FunctionDescriptor*>& referenceTable) {

  // construct ref_cell_bitmap
  uint64_t ref_cell_bitmap = 0;
  for (size_t i = 0; i < attributes.size(); i++) {
    if (!IsSpecialIndex(attributes[i])) {
      ref_cell_bitmap |= (0x1UL << i);
    }
  }

  // construct vtable
  FuncDescVec vtable(methods.size());
  for (vector<uint32_t>::const_iterator it = methods.begin();
       it != methods.end(); ++it) {
    VENOM_CHECK_RANGE(*it, referenceTable.size());
    vtable.push_back(referenceTable[*it]);
  }

  VENOM_CHECK_RANGE(ctor, referenceTable.size());
  return new venom_class_object(
      name,
      sizeof(venom_object), // all user objects have base of venom_object size
      attributes.size(),
      ref_cell_bitmap,
      venom_object::ObjClassTable.cppInit,
      venom_object::ObjClassTable.cppRelease,
      referenceTable[ctor],
      vtable);
}

Label*
CodeGenerator::newLabel() {
  Label *ret = new Label;
  labels.push_back(ret);
  return ret;
}

void
CodeGenerator::bindLabel(Label* label) {
  assert(!label->isBound());
  label->index = instructions.size();
  instToLabels[label->index] = label;
}

size_t
CodeGenerator::createLocalVariable(Symbol* symbol, bool& create) {
  return local_variable_pool.create(symbol, create);
}

size_t
CodeGenerator::createConstant(const Constant& constant, bool& create) {
  return constant_pool.create(constant, create);
}

size_t
CodeGenerator::enterLocalClass(ClassSymbol* symbol, bool& create) {
  // assert local class
  assert(symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()));
  return class_reference_table.createLocal(symbol, create);
}

size_t
CodeGenerator::enterExternalClass(ClassSymbol* symbol, bool& create) {
  // assert external class
  assert(!symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()));
  return class_reference_table.createExternal(symbol, create);
}

size_t
CodeGenerator::enterClass(ClassSymbol* symbol, bool& create) {
  return symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()) ?
     enterLocalClass(symbol, create) : enterExternalClass(symbol, create);
}

size_t
CodeGenerator::enterLocalFunction(FuncSymbol* symbol, bool& create) {
  // assert local function
  assert(symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()));
  size_t idx = func_reference_table.createLocal(symbol, create);

  // create func start label
  if (create) {
    Label *start = newBoundLabel();
    instToFuncLabels[start->index] = make_pair(start, symbol);
    funcIdxToLabels[func_pool.vec.size() - 1] = start;
  }

  return idx;
}

size_t
CodeGenerator::enterExternalFunction(FuncSymbol* symbol, bool& create) {
  // assert external function
  assert(!symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()));
  return func_reference_table.createExternal(symbol, create);
}

size_t
CodeGenerator::enterFunction(FuncSymbol* symbol, bool& create) {
  return symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()) ?
     enterLocalFunction(symbol, create) :
     enterExternalFunction(symbol, create);
}

void
CodeGenerator::emitInst(SymbolicInstruction::Opcode opcode) {
  SymbolicInstruction *inst = new SymbolicInstruction(opcode);
  instructions.push_back(inst);
}

void
CodeGenerator::emitInstU32(SymbolicInstruction::Opcode opcode, uint32_t n0) {
  SInstU32 *inst = new SInstU32(opcode, n0);
  instructions.push_back(inst);
}

//void
//CodeGenerator::emitInstI32(SymbolicInstruction::Opcode opcode, int32_t n0) {
//  SInstI32 *inst = new SInstI32(opcode, n0);
//  instructions.push_back(inst);
//}

void
CodeGenerator::emitInstLabel(SymbolicInstruction::Opcode opcode, Label* label) {
  SInstLabel *inst = new SInstLabel(opcode, label);
  instructions.push_back(inst);
}

void
CodeGenerator::emitInstI64(SymbolicInstruction::Opcode opcode, int64_t n0) {
  SInstI64 *inst = new SInstI64(opcode, n0);
  instructions.push_back(inst);
}

void
CodeGenerator::emitInstDouble(SymbolicInstruction::Opcode opcode, double n0) {
  SInstDouble *inst = new SInstDouble(opcode, n0);
  instructions.push_back(inst);
}

void
CodeGenerator::emitInstBool(SymbolicInstruction::Opcode opcode, bool n0) {
  SInstBool *inst = new SInstBool(opcode, n0);
  instructions.push_back(inst);
}

static inline uint32_t PrimitiveTypeToIndex(Type* type) {
  assert(type->isPrimitive() || type->isVoid());
  if (type->isInt()) return ClassSignature::IntIndex;
  if (type->isFloat()) return ClassSignature::FloatIndex;
  if (type->isBool()) return ClassSignature::BoolIndex;
  if (type->isVoid()) return ClassSignature::VoidIndex;
  VENOM_NOT_REACHED;
}

size_t
CodeGenerator::getClassRefIndexFromType(Type* type) {
  VENOM_ASSERT_NOT_NULL(type);
  ClassSymbol* sym = type->getClassSymbol();
  VENOM_ASSERT_NOT_NULL(sym);

  // check if special primitive
  if (type->isPrimitive() || type->isVoid()) {
    return PrimitiveTypeToIndex(type);
  } else {
    size_t idx;
    bool res = class_reference_table.find(sym, idx);
    // TODO: do we need to possibly insert external refs here?
    assert(res);
    return idx;
  }
}

ObjectCode*
CodeGenerator::createObjectCode() {
  assert(ownership);
  ownership = false;

  // build the class signature pool
  vector<ClassSignature> classSigs;
  classSigs.reserve(class_pool.vec.size());
  for (vector<ClassSymbol*>::iterator it = class_pool.vec.begin();
       it != class_pool.vec.end(); ++it) {

    vector<Symbol*> attributes;
    vector<FuncSymbol*> methods;
    ClassSymbol *csym = *it;
    csym->linearizedOrder(attributes, methods);

    vector<uint32_t> attrVec(attributes.size());
    for (vector<Symbol*>::iterator it = attributes.begin();
         it != attributes.end(); ++it) {
      attrVec.push_back(
          getClassRefIndexFromType(
            (*it)->getInstantiatedType()->getType()));
    }

    vector<uint32_t> methVec(methods.size());
    for (vector<FuncSymbol*>::iterator it = methods.begin();
         it != methods.end(); ++it) {
      size_t idx;
      bool res = func_reference_table.find(*it, idx);
      assert(res);
      methVec.push_back(idx);
    }

    // find ctor
    TypeTranslator t;
    FuncSymbol* ctor =
      csym->getClassSymbolTable()->findFuncSymbol(
          "<ctor>", SymbolTable::NoRecurse, t);
    VENOM_ASSERT_NOT_NULL(ctor);
    size_t ctorIdx;
    bool res = func_reference_table.find(ctor, ctorIdx);
    assert(res);

    classSigs.push_back(
        ClassSignature(csym->getName(), attrVec, ctorIdx, methVec));
  }

  // build the function signature pool
  vector<FunctionSignature> funcSigs;
  funcSigs.reserve(func_pool.vec.size());
  size_t idx = 0;
  for (vector<FuncSymbol*>::iterator it = func_pool.vec.begin();
       it != func_pool.vec.end(); ++it, ++idx) {

    FuncSymbol *fsym = *it;

    vector<uint32_t> paramVec(fsym->getParams().size());
    for (vector<InstantiatedType*>::iterator it = fsym->getParams().begin();
         it != fsym->getParams().end(); ++it) {
      paramVec.push_back(
          getClassRefIndexFromType((*it)->getType()));
    }

    funcSigs.push_back(
        FunctionSignature(
          fsym->getName(),
          paramVec,
          getClassRefIndexFromType(fsym->getReturnType()->getType()),
          funcIdxToLabels[idx]->index));
  }

  return new ObjectCode(
      ctx->getFullModuleName(),
      constant_pool.vec,
      classSigs,
      class_reference_table.vec,
      funcSigs,
      func_reference_table.vec,
      instructions,
      labels);
}

void
CodeGenerator::printDebugStream() {
  cerr << "; venom bytecode v0.1" << endl;
  cerr << "; module: " << ctx->getFullModuleName() << endl << endl;

  cerr << "; constant pool" << endl;
  for (size_t i = 0; i < constant_pool.vec.size(); i++) {
    cerr << i << ": " << constant_pool.vec[i] << endl;
  }
  cerr << endl;

  cerr << "; class pool" << endl;
  for (size_t i = 0; i < class_pool.vec.size(); i++) {
    cerr << i << ": ";
    ClassSymbol* csym = class_pool.vec[i];
    cerr << csym->getName() << endl;
  }
  cerr << endl;

  cerr << "; class reference table" << endl;
  for (size_t i = 0; i < class_reference_table.vec.size(); i++) {
    cerr << i << ": .classref " << class_reference_table.vec[i] << endl;
  }
  cerr << endl;

  cerr << "; function pool" << endl;
  for (size_t i = 0; i < func_pool.vec.size(); i++) {
    cerr << i << ": ";
    FuncSymbol* fsym = func_pool.vec[i];
    cerr << fsym->getName() << endl;
  }
  cerr << endl;

  cerr << "; function reference table" << endl;
  for (size_t i = 0; i < func_reference_table.vec.size(); i++) {
    cerr << i << ": .funcref " << func_reference_table.vec[i] << endl;
  }
  cerr << endl;

  size_t index = 0;
  for (vector<SymbolicInstruction*>::iterator it = instructions.begin();
       it != instructions.end(); ++it, ++index) {
    InstLabelSymbolPairMap::iterator iit = instToFuncLabels.find(index);
    if (iit != instToFuncLabels.end()) {
      cerr << iit->second.second->getName() << ":" << endl;
    } else {
      InstLabelMap::iterator iit = instToLabels.find(index);
      if (iit != instToLabels.end()) {
        cerr << *iit->second << ":" << endl;
      }
    }
    cerr << "  ";
    (*it)->printDebug(cerr);
  }
}

}
}
