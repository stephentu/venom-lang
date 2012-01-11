#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/return.h>
#include <ast/statement/stmtexpr.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
StmtExprNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  InstantiatedType *retType = expr->typeCheck(ctx);
  if (!expected) return;
  if (!retType->isSubtypeOf(*expected)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() +
        ", got type " + retType->stringify());
  }
}

void
StmtExprNode::codeGen(CodeGenerator& cg) {
  expr->codeGen(cg);
  cg.emitInst(expr->getStaticType()->isRefCounted() ?
      Instruction::POP_CELL_REF : Instruction::POP_CELL);
}

ASTNode*
StmtExprNode::rewriteReturn(SemanticContext* ctx) {
  ReturnNode *ret = new ReturnNode(ASTNode::Clone(expr));
  ret->initSymbolTable(symbols);
  return ret;
}

StmtExprNode*
StmtExprNode::cloneImpl() {
  return new StmtExprNode(expr->clone());
}

}
}
