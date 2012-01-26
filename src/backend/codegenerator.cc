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
  FuncDescVec vtable;
  vtable.reserve(methods.size());
  for (vector<uint32_t>::const_iterator it = methods.begin();
       it != methods.end(); ++it) {
    VENOM_CHECK_RANGE(*it, referenceTable.size());
    assert(referenceTable[*it]->getNumArgs() >= 1); // since its a method
    vtable.push_back(referenceTable[*it]);
  }

  if (ctor != -1) VENOM_CHECK_RANGE(size_t(ctor), referenceTable.size());
  return new venom_class_object(
      name,
      sizeof(venom_object), // all user objects have base of venom_object size
      attributes.size(),
      ref_cell_bitmap,
      venom_object::ObjClassTable().cppInit,
      venom_object::ObjClassTable().cppRelease,
      ctor == -1 ? &venom_object::CtorDescriptor() : referenceTable[ctor],
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

Symbol*
CodeGenerator::createTemporaryVariable() {
  if (available_temporary_symbols.empty()) {
    Symbol* ret = new Symbol("", NULL, NULL, NULL);
    temporary_symbols.insert(ret);
    return ret;
  } else {
    set<Symbol*>::iterator it = available_temporary_symbols.begin();
    Symbol* ret = *it;
    available_temporary_symbols.erase(it);
    return ret;
  }
}

void
CodeGenerator::returnTemporaryVariable(Symbol* symbol) {
  // must have created a temp variable using
  // createTemporaryVariable()
  assert(temporary_symbols.find(symbol) != temporary_symbols.end());

  // must not be a symbol which is already considered
  // available
  assert(available_temporary_symbols.find(symbol) ==
         available_temporary_symbols.end());

  available_temporary_symbols.insert(symbol);
}

size_t
CodeGenerator::createConstant(const Constant& constant, bool& create) {
  return constant_pool.create(constant, create);
}

size_t
CodeGenerator::enterLocalClass(ClassSymbol* symbol, bool& create) {
  // assert local class
  assert(isLocalSymbol(symbol));
  assert(symbol->getTypeParams().empty());
  assert(symbol->isCodeGeneratable());
  return class_reference_table.createLocal(symbol, create);
}

size_t
CodeGenerator::enterExternalClass(ClassSymbol* symbol, bool& create) {
  // assert external class
  assert(!isLocalSymbol(symbol));
  assert(symbol->getTypeParams().empty());
  assert(symbol->isCodeGeneratable());
  return class_reference_table.createExternal(symbol, create);
}

size_t
CodeGenerator::enterLocalClass(InstantiatedType* klass, bool& create) {
  return enterLocalClass(resolveToSymbol(klass), create);
}

size_t
CodeGenerator::enterExternalClass(InstantiatedType* klass, bool& create) {
  return enterExternalClass(resolveToSymbol(klass), create);
}

size_t
CodeGenerator::enterClass(InstantiatedType* klass, bool& create) {
  ClassSymbol* symbol = resolveToSymbol(klass);
  return isLocalSymbol(symbol) ?
     enterLocalClass(symbol, create) : enterExternalClass(symbol, create);
}

size_t
CodeGenerator::enterLocalFunction(FuncSymbol* symbol, bool& create) {
  // assert local function
  assert(symbol->getDefinedSymbolTable()->belongsTo(
         ctx->getModuleRoot()->getSymbolTable()));
  assert(symbol->isCodeGeneratable());
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
  assert(symbol->isCodeGeneratable());
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
CodeGenerator::getClassRefIndexFromType(InstantiatedType* type) {
  VENOM_ASSERT_NOT_NULL(type);

  // check if special primitive
  if (type->isPrimitive() || type->isVoid()) {
    return PrimitiveTypeToIndex(type->getType());
  } else {
    bool create;
    return enterClass(type, create);
  }
}

bool
CodeGenerator::isLocalSymbol(const BaseSymbol* symbol) const {
  return (symbol->getDefinedSymbolTable()->belongsTo(
            ctx->getModuleRoot()->getSymbolTable()) ||
          symbol->getDefinedSymbolTable() == ctx->getRootSymbolTable());
}

ClassSymbol*
CodeGenerator::resolveToSymbol(InstantiatedType* klass) {
  return klass->findSpecializedClassSymbol();
}

void
CodeGenerator::createObjectCodeAndSet(analysis::SemanticContext* ctx) {
  ctx->setObjectCode(createObjectCode());
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

    assert((*it)->isCodeGeneratable());

    vector<Symbol*> attributes;
    vector<FuncSymbol*> methods;
    ClassSymbol *csym = *it;
    csym->linearizedOrder(attributes, methods);

    vector<uint32_t> attrVec;
    attrVec.reserve(attributes.size());
    for (vector<Symbol*>::iterator it = attributes.begin();
         it != attributes.end(); ++it) {
      attrVec.push_back(
          getClassRefIndexFromType(
            (*it)->getInstantiatedType()));
    }

    vector<uint32_t> methVec;
    methVec.reserve(methods.size());
    for (vector<FuncSymbol*>::iterator it = methods.begin();
         it != methods.end(); ++it) {
      bool create;
      size_t idx = enterFunction(*it, create);
      methVec.push_back(idx);
    }

    // find ctor
    int32_t ctorIdx = -1;

    if (!csym->isModuleClassSymbol()) {
      TypeTranslator t;
      FuncSymbol* ctor =
        csym->getClassSymbolTable()->findFuncSymbol(
            "<ctor>", SymbolTable::NoRecurse, t);
      VENOM_ASSERT_NOT_NULL(ctor);
      size_t idx;
      bool res = func_reference_table.find(ctor, idx);
      VENOM_ASSERT(res);
      ctorIdx = idx;
    }

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

    vector<uint32_t> paramVec;
    paramVec.reserve(fsym->getParams().size());
    for (vector<InstantiatedType*>::iterator it = fsym->getParams().begin();
         it != fsym->getParams().end(); ++it) {
      paramVec.push_back(
          getClassRefIndexFromType(*it));
    }

    if (MethodSymbol* ms = dynamic_cast<MethodSymbol*>(fsym)) {
      funcSigs.push_back(
          FunctionSignature(
            ms->getClassSymbol()->getName(),
            fsym->getName(),
            paramVec,
            getClassRefIndexFromType(fsym->getReturnType()),
            funcIdxToLabels[idx]->index));
    } else {
      funcSigs.push_back(
          FunctionSignature(
            fsym->getName(),
            paramVec,
            getClassRefIndexFromType(fsym->getReturnType()),
            funcIdxToLabels[idx]->index));
    }
  }

  // build name -> inst offset map
  ObjectCode::NameOffsetMap nameOffsetMap;
  for (InstLabelSymbolPairMap::iterator it = instToFuncLabels.begin();
       it != instToFuncLabels.end(); ++it) {
    nameOffsetMap[it->second.second->getName()] = it->first;
  }

  return new ObjectCode(
      ctx->getFullModuleName(),
      constant_pool.vec,
      classSigs,
      class_reference_table.vec,
      funcSigs,
      func_reference_table.vec,
      instructions,
      nameOffsetMap,
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
    cerr << fsym->getFullName() << endl;
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
      BaseSymbol* bs = iit->second.second;
      if (MethodSymbol* ms = dynamic_cast<MethodSymbol*>(bs)) {
        cerr << ms->getClassSymbol()->getName() << "."
             << ms->getName() << ":" << endl;
      } else {
        cerr << bs->getName() << ":" << endl;
      }
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
