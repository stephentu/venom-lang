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

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/ifstmt.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void IfStmtNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  cond->typeCheck(ctx);
  true_branch->typeCheck(ctx, expected);
  false_branch->typeCheck(ctx, expected);
}

ASTNode*
IfStmtNode::rewriteReturn(SemanticContext* ctx) {
  ASTNode *trueRep = true_branch->rewriteReturn(ctx);
  VENOM_ASSERT_NULL(trueRep);
  ASTNode *falseRep = false_branch->rewriteReturn(ctx);
  VENOM_ASSERT_NULL(falseRep);
  return NULL;
}

void IfStmtNode::codeGen(CodeGenerator& cg) {
  Label *done = cg.newLabel();
  Label *falseBranch = cg.newLabel();

  cond->codeGen(cg);
  Instruction::Opcode op;
  if (cond->getStaticType()->isInt()) {
    op = Instruction::BRANCH_Z_INT;
  } else if (cond->getStaticType()->isFloat()) {
    op = Instruction::BRANCH_Z_FLOAT;
  } else if (cond->getStaticType()->isBool()) {
    op = Instruction::BRANCH_Z_BOOL;
  } else {
    op = Instruction::BRANCH_Z_REF;
  }
  cg.emitInstLabel(op, falseBranch);
  true_branch->codeGen(cg);
  cg.emitInstLabel(Instruction::JUMP, done);
  cg.bindLabel(falseBranch);
  false_branch->codeGen(cg);
  cg.bindLabel(done);
}

IfStmtNode*
IfStmtNode::cloneImpl(CloneMode::Type type) {
  return new IfStmtNode(
      cond->clone(type), true_branch->clone(type), false_branch->clone(type));
}

ASTStatementNode*
IfStmtNode::cloneForLiftImpl(LiftContext& ctx) {
  return new IfStmtNode(
      cond->cloneForLift(ctx), true_branch->cloneForLift(ctx), false_branch->cloneForLift(ctx));
}

IfStmtNode*
IfStmtNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new IfStmtNode(
      cond->cloneForTemplate(t),
      true_branch->cloneForTemplate(t),
      false_branch->cloneForTemplate(t));
}

}
}
