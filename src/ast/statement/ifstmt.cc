#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/ifstmt.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void IfStmtNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  cond->typeCheck(ctx);
  true_branch->typeCheck(ctx, expected);
  false_branch->typeCheck(ctx, expected);
}

}
}
