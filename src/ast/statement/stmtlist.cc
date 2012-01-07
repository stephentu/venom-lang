#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

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

}
}
