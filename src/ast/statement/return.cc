#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void ReturnNode::registerSymbol(SemanticContext* ctx) {
  FuncDeclNode *fdn = getParentFuncNode();
  if (!fdn) {
    throw SemanticViolationException(
        "return statement must be in context of function scope");
  }
}

void ReturnNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  FuncDeclNode *fdn = getParentFuncNode();
  assert(fdn);
  TypeTranslator t;
  FuncSymbol* fs =
    fdn->getSymbolTable()->findFuncSymbol(fdn->getName(), false, t);
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

}
}
