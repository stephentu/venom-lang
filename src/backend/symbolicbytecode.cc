/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
    return new InstFormatIPtr(opcode,
                              intptr_t(resTable.getClassRefTable()[value]));
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
