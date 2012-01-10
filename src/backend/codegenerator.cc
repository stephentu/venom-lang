#include <stdexcept>

#include <ast/statement/node.h>
#include <backend/codegenerator.h>
#include <backend/linker.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace backend {

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
CodeGenerator::createConstant(const string& constant, bool& create) {
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

ObjectCode*
CodeGenerator::createObjectCode() {
  assert(ownership);
  ownership = false;
  return new ObjectCode(
      constant_pool.vec,
      class_reference_table.vec,
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
    // TODO: escape string
    cerr << ".const" << i << " \"" << constant_pool.vec[i] << "\"" << endl;
  }
  cerr << endl;

  cerr << "; class pool" << endl;
  // TODO: implement me
  cerr << endl;

  cerr << "; class reference table" << endl;
  // TODO: implement me
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
    cerr << ".funcref ";
    SymbolReference& sref = func_reference_table.vec[i];
    if (sref.isLocal()) {
      cerr << ".local " << sref.getLocalIndex() << endl;
    } else {
      cerr << ".extern " << sref.getFullName() << endl;
    }
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
        cerr << "label_" << iit->second->index << ":" << endl;
      }
    }
    cerr << "  ";
    (*it)->printDebug(cerr);
  }
}

}
}
