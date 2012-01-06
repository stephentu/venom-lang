#include <cassert>

#include <backend/codegenerator.h>
#include <backend/symbolicbytecode.h>
#include <backend/vm.h>
#include <util/macros.h>

using namespace std;

namespace venom {
namespace backend {

Instruction* SymbolicInstruction::resolve(size_t pos, ResolutionTable& resTable) {
  // TODO: assert that the opcode is valid for this type
  return new Instruction(opcode);
}

Instruction* SInstU32::resolve(size_t pos, ResolutionTable& resTable) {
  switch (opcode) {
  case Instruction::PUSH_CONST:
    return new InstFormatU32(opcode, resTable.getConstantTable()[value]);
  case Instruction::ALLOC_OBJ:
    return new InstFormatU32(opcode, resTable.getClassRefTable()[value]);
  case Instruction::CALL:
  case Instruction::CALL_NATIVE:
    return new InstFormatIPtr(opcode,
                              intptr_t(resTable.getFuncRefTable()[value]));
  case Instruction::CALL_VIRTUAL:
  case Instruction::LOAD_LOCAL_VAR:
  case Instruction::LOAD_LOCAL_VAR_REF:
  case Instruction::STORE_LOCAL_VAR:
  case Instruction::STORE_LOCAL_VAR_REF:
  case Instruction::GET_ATTR_OBJ:
  case Instruction::GET_ATTR_OBJ_REF:
  case Instruction::SET_ATTR_OBJ:
  case Instruction::SET_ATTR_OBJ_REF:
  case Instruction::DUP:
  case Instruction::DUP_REF:
    return new InstFormatU32(opcode, value);
  default: assert(false);
  }
  VENOM_NOT_REACHED;
}

Instruction* SInstLabel::resolve(size_t pos, ResolutionTable& resTable) {
  switch (opcode) {
  case Instruction::JUMP:
  case Instruction::BRANCH_Z_INT:
  case Instruction::BRANCH_Z_FLOAT:
  case Instruction::BRANCH_Z_BOOL:
  case Instruction::BRANCH_Z_REF:
  case Instruction::BRANCH_NZ_INT:
  case Instruction::BRANCH_NZ_FLOAT:
  case Instruction::BRANCH_NZ_BOOL:
  case Instruction::BRANCH_NZ_REF:
    return new InstFormatI32(opcode, value->calcOffset(pos) - 1);
  default: assert(false);
  }
  VENOM_NOT_REACHED;
}

Instruction* SInstI64::resolve(size_t pos, ResolutionTable& resTable) {
  assert(opcode == Instruction::PUSH_CELL_INT);
  return new InstFormatC(opcode, value);
}

Instruction* SInstDouble::resolve(size_t pos, ResolutionTable& resTable) {
  assert(opcode == Instruction::PUSH_CELL_FLOAT);
  return new InstFormatC(opcode, value);
}

Instruction* SInstBool::resolve(size_t pos, ResolutionTable& resTable) {
  assert(opcode == Instruction::PUSH_CELL_BOOL);
  return new InstFormatC(opcode, value);
}

void SInstLabel::printDebug(ostream& o) {
  o << Instruction::stringify(opcode) << " label_" << value->getIndex()
    << endl;
}

}
}
