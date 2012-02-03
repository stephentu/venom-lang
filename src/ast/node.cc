#include <cassert>

#include <analysis/symboltable.h>

#include <ast/expression/node.h>
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

ASTNode::~ASTNode() {}

FuncDeclNode* ASTNode::getEnclosingFuncNode() {
  ASTNode *cur = this;
  while (cur) {
    if (FuncDeclNode *fdn = dynamic_cast<FuncDeclNode*>(cur)) {
      return fdn;
    }
    cur = cur->getSymbolTable()->getOwner();
  }
  return NULL;
}

const FuncDeclNode* ASTNode::getEnclosingFuncNode() const {
  return const_cast<ASTNode*>(this)->getEnclosingFuncNode();
}

ClassDeclNode* ASTNode::getEnclosingClassNode() {
  assert(symbols);
  ASTNode *cur = this;
  while (cur) {
    if (ClassDeclNode *cdn = dynamic_cast<ClassDeclNode*>(cur)) {
      return cdn;
    }
    cur = cur->getSymbolTable()->getOwner();
  }
  return NULL;
}

const ClassDeclNode* ASTNode::getEnclosingClassNode() const {
  return const_cast<ASTNode*>(this)->getEnclosingClassNode();
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
