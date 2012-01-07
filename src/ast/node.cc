#include <cassert>

#include <analysis/symboltable.h>

#include <ast/expression/node.h>
#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/node.h>
#include <ast/node.h>

#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

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
      kid->initSymbolTable(symbols->newChildScope(this));
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

ASTNode*
ASTNode::rewriteLocal(SemanticContext* ctx) {
  for (size_t i = 0; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    if (!kid) continue;
    ASTNode* rep = kid->rewriteLocal(ctx);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete rep;
    }
  }
  return NULL;
}

void
ASTNode::codeGen(CodeGenerator& cg) {
  forchild (kid) {
    if (!kid) continue;
    kid->codeGen(cg);
  } endfor
}

ASTNode*
ASTNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  replacement->initSymbolTable(getSymbolTable());
  replacement->semanticCheck(ctx);
  if (ASTStatementNode *stmt =
        dynamic_cast<ASTStatementNode*>(replacement)) {
    stmt->typeCheck(ctx);
  } else if (ASTExpressionNode *expr =
               dynamic_cast<ASTExpressionNode*>(replacement)) {
    expr->typeCheck(ctx);
  }
  return replacement;
}

}
}
