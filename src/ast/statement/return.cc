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

#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void ReturnNode::registerSymbol(SemanticContext* ctx) {
  FuncDeclNode *fdn = getEnclosingFuncNode();
  if (!fdn) {
    throw SemanticViolationException(
        "return statement must be in context of function scope");
  }
}

void ReturnNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  FuncDeclNode *fdn = getEnclosingFuncNode();
  assert(fdn);
  TypeTranslator t;
  FuncSymbol* fs =
    fdn->getSymbolTable()->findFuncSymbol(
        fdn->getName(), SymbolTable::NoRecurse, t);
  InstantiatedType* expRetType = t.translate(ctx, fs->getReturnType());
  InstantiatedType *retType =
    expr ? expr->typeCheck(ctx) : InstantiatedType::VoidType;
  if (!retType->isSubtypeOf(*expRetType)) {
    throw TypeViolationException(
        "Expected type " + expRetType->stringify() +
        ", got type " + retType->stringify());
  }
  if (!expected) return;
  if (!retType->isSubtypeOf(*expected)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() +
        ", got type " + retType->stringify());
  }
}

void
ReturnNode::codeGen(CodeGenerator& cg) {
  if (!expr) {
    cg.emitInst(Instruction::PUSH_CELL_NIL);
  } else {
    expr->codeGen(cg);
  }
  cg.emitInst(Instruction::RET);
}

ReturnNode*
ReturnNode::cloneImpl(CloneMode::Type type) {
  return new ReturnNode(expr ? expr->clone(type) : NULL);
}

ASTStatementNode*
ReturnNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ReturnNode(expr ? expr->cloneForLift(ctx) : NULL);
}

ReturnNode*
ReturnNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ReturnNode(expr ? expr->cloneForTemplate(t) : NULL);
}

}
}
