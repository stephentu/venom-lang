#include <ast/expression/doubleliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
DoubleLiteralNode::typeCheckImpl(SemanticContext*  ctx,
                                 InstantiatedType* expected,
                                 const InstantiatedTypeVec& typeParamArgs) {
  return InstantiatedType::FloatType;
}

void
DoubleLiteralNode::codeGen(CodeGenerator& cg) {
  cg.emitInstDouble(Instruction::PUSH_CELL_FLOAT, value);
}

DoubleLiteralNode*
DoubleLiteralNode::cloneImpl() {
  return new DoubleLiteralNode(value);
}

ASTExpressionNode*
DoubleLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new DoubleLiteralNode(value);
}

DoubleLiteralNode*
DoubleLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new DoubleLiteralNode(value);
}


}
}
