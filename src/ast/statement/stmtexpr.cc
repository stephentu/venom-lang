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
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
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

#include <ast/statement/return.h>
#include <ast/statement/stmtexpr.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
StmtExprNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  InstantiatedType *retType = expr->typeCheck(ctx);
  if (!expected || expected->isVoid()) return;
  if (!retType->isSubtypeOf(*expected)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() +
        ", got type " + retType->stringify());
  }
}

void
StmtExprNode::codeGen(CodeGenerator& cg) {
  expr->codeGen(cg);
  cg.emitInst(expr->getStaticType()->isRefCounted() ?
      Instruction::POP_CELL_REF : Instruction::POP_CELL);
}

ASTNode*
StmtExprNode::rewriteReturn(SemanticContext* ctx) {
  return replace(ctx, new ReturnNode(expr->clone(CloneMode::Semantic)));
}

StmtExprNode*
StmtExprNode::cloneImpl(CloneMode::Type type) {
  return new StmtExprNode(expr->clone(type));
}

ASTStatementNode*
StmtExprNode::cloneForLiftImpl(LiftContext& ctx) {
  return new StmtExprNode(expr->cloneForLift(ctx));
}

StmtExprNode*
StmtExprNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new StmtExprNode(expr->cloneForTemplate(t));
}

}
}
