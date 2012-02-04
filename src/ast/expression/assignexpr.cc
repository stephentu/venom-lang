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

#include <ast/expression/assignexpr.h>
#include <ast/expression/variable.h>
#include <ast/statement/assign.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

BaseSymbol*
AssignExprNode::getSymbol() {
  return variable->getSymbol();
}

void
AssignExprNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  if (var) {
    AssignNode::RegisterVariableLHS(ctx, symbols, var, this);
  }
}

void
AssignExprNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // TODO: re-factor code duplication between here and
  // ast/statement/assign.cc

  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);

  // register now, before recursing on variable
  if (doRegister) registerSymbol(ctx);

  // now recurse on variable
  variable->semanticCheckImpl(ctx, true);
}

ASTNode*
AssignExprNode::rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const set<BaseSymbol*>& refs) {

#ifndef NDEBUG
  // assert this isn't the decl of some non local ref
  BaseSymbol* psym = variable->getSymbol();
  if (psym) {
    set<BaseSymbol*>::const_iterator it = refs.find(psym);
    if (it != refs.end()) {
      VENOM_ASSERT_TYPEOF_PTR(Symbol, psym);
      Symbol* sym = static_cast<Symbol*>(psym);
      assert(sym->getDecl() != this);
    }
  }
#endif /* NDEBUG */

  return ASTExpressionNode::rewriteAfterLift(liftMap, refs);
}

InstantiatedType*
AssignExprNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  assert(value);
  assert(typeParamArgs.empty());
  AssignNode::decl_either decl(this);
  return AssignNode::TypeCheckAssignment(ctx, symbols, variable, value, decl);
}

void
AssignExprNode::codeGen(CodeGenerator& cg) {
  AssignNode::CodeGenAssignment(cg, variable, value);
  // need to leave the variable on the stack
  variable->codeGen(cg);
}

AssignExprNode*
AssignExprNode::cloneImpl(CloneMode::Type type) {
  return new AssignExprNode(variable->clone(type), value->clone(type));
}

ASTExpressionNode*
AssignExprNode::cloneForLiftImpl(LiftContext& ctx) {
  return new AssignExprNode(variable->cloneForLift(ctx), value->cloneForLift(ctx));
}

AssignExprNode*
AssignExprNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AssignExprNode(
      variable->cloneForTemplate(t), value->cloneForTemplate(t));
}

}
}
