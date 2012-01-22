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
