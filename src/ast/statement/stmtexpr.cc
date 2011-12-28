#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/stmtexpr.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void StmtExprNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  InstantiatedType *retType = expr->typeCheck(ctx);
  if (!expected) return;
  if (!retType->isSubtypeOf(*expected)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() +
        ", got type " + retType->stringify());
  }
}

}
}
