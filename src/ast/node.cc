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

#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/node.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/outer.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/node.h>

#include <ast/node.h>

#include <backend/codegenerator.h>

#include <util/macros.h>
#include <util/stl.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

bool
LiftContext::isLiftingFunction() const {
  return dynamic_cast<FuncSymbol*>(curLiftSym);
}

bool
LiftContext::isLiftingClass() const {
  return dynamic_cast<ClassSymbol*>(curLiftSym);
}

string
LiftContext::refParamName(Symbol* nonLocSym) {
  bool create;
  size_t idx = refs.create(nonLocSym, create);
  return RefParamName(nonLocSym, idx);
}

string
LiftContext::RefParamName(Symbol* nonLocSym, size_t pos) {
  return nonLocSym->getName() + "$refparam_" + util::stringify(pos);
}

bool
LiftContext::useExplicitTypeForClassSymbol(ClassSymbol* csym) {
  return csym
    ->getDefinedSymbolTable()
    ->belongsTo(curLiftSym->getDefinedSymbolTable());
}

ASTNode::~ASTNode() {}

FuncDeclNode* ASTNode::getEnclosingFuncNode() {
  assert(symbols);
  ASTNode *cur = getSymbolTable()->getOwner();
  while (cur) {
    if (FuncDeclNode *fdn = dynamic_cast<FuncDeclNode*>(cur)) {
      return fdn;
    }
    cur = cur->getSymbolTable()->getOwner();
  }
  return NULL;
}

ClassDeclNode* ASTNode::getEnclosingClassNode() {
  assert(symbols);
  ASTNode *cur = getSymbolTable()->getOwner();
  while (cur) {
    if (ClassDeclNode *cdn = dynamic_cast<ClassDeclNode*>(cur)) {
      return cdn;
    }
    cur = cur->getSymbolTable()->getOwner();
  }
  return NULL;
}

ASTStatementNode* ASTNode::getEnclosingTypeParameterizedNode() {
  assert(symbols);
  ASTNode *cur = getSymbolTable()->getOwner();
  while (cur) {
    if (cur->isTypeParameterized()) {
      VENOM_ASSERT_TYPEOF_PTR(ASTStatementNode, cur);
      return static_cast<ASTStatementNode*>(cur);
    }
    cur = cur->getSymbolTable()->getOwner();
  }
  return NULL;
}

void ASTNode::initSymbolTable(SymbolTable* symbols) {
  assert(this->symbols == NULL);
  this->symbols = symbols;

  forchild (kid) {
    if (!kid) continue;
    if (needsNewScope(i)) {
      kid->initSymbolTable(symbols->newChildScope(this, kid));
    } else {
      kid->initSymbolTable(symbols);
    }
  } endfor
}

void
ASTNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }
  forchild (kid) {
    if (!kid) continue;
    kid->semanticCheckImpl(ctx, true);
  } endfor
}

// TODO: use macros and/or templates to avoid code duplication

void
ASTNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  forchild (kid) {
    if (!kid) continue;
    kid->collectSpecialized(ctx, t, callback);
  } endfor
}

void
ASTNode::collectNonLocalRefs(LiftContext& ctx) {
  forchild (kid) {
    if (!kid) continue;
    kid->collectNonLocalRefs(ctx);
  } endfor
}

ASTNode*
ASTNode::rewriteAfterLift(const LiftContext::LiftMap& liftMap,
                          const set<BaseSymbol*>& refs) {
  for (size_t i = 0; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    if (!kid || kid->isTypeParameterized()) continue;
    ASTNode* rep = kid->rewriteAfterLift(liftMap, refs);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete kid;
    }
  }
  return NULL;
}

ASTNode*
ASTNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  for (size_t i = 0; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    if (!kid || kid->isTypeParameterized()) continue;
    ASTNode* rep = kid->rewriteLocal(ctx, mode);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete kid;
    }
  }
  return NULL;
}

ASTNode*
ASTNode::clone(CloneMode::Type type) {
  ASTNode* copy = cloneImpl(type);
  assert(copy);
  cloneSetState(copy);
  return copy;
}

ASTNode*
ASTNode::cloneForTemplate(const TypeTranslator& t) {
  ASTNode* copy = cloneForTemplateImpl(t);
  assert(copy);
  cloneSetState(copy);
  return copy;
}

ASTNode*
ASTNode::cloneForLift(LiftContext& ctx) {
  assert(!isTypeParameterized());
  ASTNode* copy = cloneForLiftImpl(ctx);
  assert(copy);
  cloneSetState(copy);
  return copy;
}

void
ASTNode::codeGen(CodeGenerator& cg) {
  forchild (kid) {
    if (!kid || kid->isTypeParameterized()) continue;
    kid->codeGen(cg);
  } endfor
}

ASTNode*
ASTNode::rewriteReturn(SemanticContext* ctx) { VENOM_NOT_REACHED; }

void
ASTNode::printStderr() const {
  const_cast<ASTNode*>(this)->print(cerr, 0);
}

static ASTExpressionNode* createAttrChain0(size_t n) {
  assert(n > 0);
  if (n == 1) return new VariableNodeParser("<outer>", NULL);
  return new OuterNode(createAttrChain0(n - 1));
}

ASTExpressionNode*
ASTNode::CreateOuterAttrChain(size_t n, const string& name) {
  if (n == 0) return new VariableNodeParser(name, NULL);
  return new AttrAccessNode(createAttrChain0(n), name);
}

ASTNode*
ASTNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  replacement->setLocationContext(getLocationContext());
  replacement->initSymbolTable(getSymbolTable());
  replacement->semanticCheck(ctx);
  if (ASTStatementNode *stmt =
        dynamic_cast<ASTStatementNode*>(replacement)) {
    VENOM_ASSERT_TYPEOF_PTR(ASTStatementNode, this);
    stmt->typeCheck(ctx);
  } else if (ASTExpressionNode *expr =
               dynamic_cast<ASTExpressionNode*>(replacement)) {
    VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, this);
    ASTExpressionNode *self = static_cast<ASTExpressionNode*>(this);
    expr->typeCheck(ctx, self->getExpectedType(), self->getTypeParamArgs());
    // TODO: assert resulting typeCheck equals the original node's typeCheck?
  }
  return replacement;
}

}
}
