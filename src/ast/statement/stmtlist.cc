#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtlist.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
StmtListNode::registerSymbol(SemanticContext* ctx) {
  if (getSymbolTable()->isModuleLevelSymbolTable()) {
    // create module type
    Type *moduleType = ctx->createModuleType(ctx->getModuleName());

    // create the module class symbol (in the *ROOT* symbol table
    // (which is the parent of this node's symbol table))
    ctx->getRootSymbolTable()->createClassSymbol(
        moduleType->getName(), getSymbolTable(), moduleType);

    // create the symbol for the module object singleton
    ctx->getRootSymbolTable()->createModuleSymbol(
        ctx->getModuleName(), getSymbolTable(), moduleType, ctx);
  }
}

void
StmtListNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  if (stmts.empty()) {
    checkExpectedType(expected);
  } else {
    for (size_t i = 0; i < stmts.size(); i++) {
      if (i == stmts.size() - 1) {
        stmts[i]->typeCheck(ctx, expected);
      } else {
        stmts[i]->typeCheck(ctx);
      }
    }
  }
}

ASTNode*
StmtListNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  if (mode != ModuleMain) return ASTNode::rewriteLocal(ctx, mode);

  if (getSymbolTable()->isModuleLevelSymbolTable()) {
    // we move all the non function/class definitions into
    // a separate function we call <main>

    StmtNodeVec mainStmts;
    for (StmtNodeVec::iterator it = stmts.begin();
         it != stmts.end();) {
      ASTStatementNode* kid = *it;
      if (dynamic_cast<FuncDeclNode*>(kid) ||
          dynamic_cast<ClassDeclNode*>(kid)) {
        ++it; continue;
      }
      it = stmts.erase(it);
      mainStmts.push_back(kid);
    }

    // create a function with mainStmts
    FuncDeclNode* mainFcn = new FuncDeclNode(
        "<main>", util::StrVec(),
        ExprNodeVec(), NULL, new StmtListNode(mainStmts));
    mainFcn->setSymbolTable(symbols);
    mainFcn->registerSymbol(ctx);

    // don't typecheck the new function (no types changed)

    // append to the stmt list
    stmts.push_back(mainFcn);
  }
  return NULL;
}

StmtListNode*
StmtListNode::cloneImpl() {
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
        ASTStatementNode::CloneFunctor()));
}

}
}
