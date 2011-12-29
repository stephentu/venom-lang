#include <cassert>

#include <analysis/symboltable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/node.h>

#include <util/macros.h>

namespace venom {
namespace ast {

ASTNode::~ASTNode() {
  if (symbols) delete symbols;
}

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

void ASTNode::initSymbolTable(analysis::SymbolTable* symbols) {
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
ASTNode::semanticCheckImpl(analysis::SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }
  forchild (kid) {
    if (!kid) continue;
    kid->semanticCheckImpl(ctx, true);
  } endfor
}

}
}
