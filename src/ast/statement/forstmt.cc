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

#include <ast/expression/variable.h>
#include <ast/statement/forstmt.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ForStmtNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  assert(doRegister);
  expr->semanticCheckImpl(ctx, true);
  registerSymbol(ctx);
  stmts->semanticCheckImpl(ctx, true);
}

void
ForStmtNode::registerSymbol(SemanticContext* ctx) {
  // put variable in the stmts symbol table
  // (of unknown type)
  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  assert(vn);
  stmts->getSymbolTable()->createSymbol(vn->getName(), NULL, this);
}

void
ForStmtNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  InstantiatedType *iterableType = expr->typeCheck(ctx);

  // for now, assume that expr must be of type list[x] or string
  // in the future, we should allow any iterable (and string will be
  // defined as an iterable[string])
  if (!iterableType->getType()->equals(*Type::ListType) &&
      !iterableType->equals(*InstantiatedType::StringType)) {
    throw TypeViolationException(
        "Expect type list or string, got " + iterableType->stringify());
  }

  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  assert(vn);

	TypeTranslator t;
	Symbol* sym =
		stmts->getSymbolTable()->findSymbol(
				vn->getName(), SymbolTable::NoRecurse, t);
	assert(sym);
  // now the type information is available, set it
  if (iterableType->getType()->equals(*Type::ListType)) {
		sym->setInstantiatedType(iterableType->getParams().front());
  } else {
    // string type
		sym->setInstantiatedType(InstantiatedType::StringType);
  }
  stmts->typeCheck(ctx);
  checkExpectedType(expected);
}

ASTNode*
ForStmtNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  for (size_t i = 1; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    assert(kid);
    ASTNode* rep = kid->rewriteLocal(ctx, mode);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete kid;
    }
  }
  return NULL;
}

ForStmtNode*
ForStmtNode::cloneImpl(CloneMode::Type type) {
  return new ForStmtNode(variable->clone(type), expr->clone(type), stmts->clone(type));
}

ASTStatementNode*
ForStmtNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ForStmtNode(variable->cloneForLift(ctx), expr->cloneForLift(ctx), stmts->cloneForLift(ctx));
}

ForStmtNode*
ForStmtNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ForStmtNode(
      variable->cloneForTemplate(t),
      expr->cloneForTemplate(t),
      stmts->cloneForTemplate(t));
}

}
}
