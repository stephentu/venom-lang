#include <ast/expression/assignexpr.h>
#include <ast/expression/variable.h>
#include <ast/statement/assign.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

BaseSymbol*
AssignExprNode::getSymbol() {
  return variable->getSymbol();
}

void
AssignExprNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  if (var) {
    AssignNode::RegisterVariableLHS(ctx, symbols, var);
  }
}

void
AssignExprNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
  // TODO: not really sure if this is correct...
}

InstantiatedType*
AssignExprNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  assert(value);
  assert(typeParamArgs.empty());
  return AssignNode::TypeCheckAssignment(ctx, symbols, variable, value);
}

void
AssignExprNode::codeGen(CodeGenerator& cg) {
  AssignNode::CodeGenAssignment(cg, variable, value);
  // need to leave the variable on the stack
  variable->codeGen(cg);
}

AssignExprNode*
AssignExprNode::cloneImpl() {
  return new AssignExprNode(variable->clone(), value->clone());
}

AssignExprNode*
AssignExprNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AssignExprNode(
      variable->cloneForTemplate(t), value->cloneForTemplate(t));
}

}
}
