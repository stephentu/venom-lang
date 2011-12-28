#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassAttrDeclNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  assert(vn);
  AssignNode::RegisterSymbolForAssignment(ctx, symbols, vn, false);
}

void
ClassAttrDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
}

void
ClassAttrDeclNode::typeCheck(SemanticContext* ctx) {
  if (value) AssignNode::TypeCheckAssignment(ctx, symbols, variable, value);
}

}
}
