#include <ast/expression/intliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
IntLiteralNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  return InstantiatedType::IntType;
}

void
IntLiteralNode::codeGen(CodeGenerator& cg) {
  cg.emitInstI64(Instruction::PUSH_CELL_INT, value);
}

IntLiteralNode*
IntLiteralNode::cloneImpl(CloneMode::Type type) {
  return new IntLiteralNode(value);
}

ASTExpressionNode*
IntLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new IntLiteralNode(value);
}

IntLiteralNode*
IntLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new IntLiteralNode(value);
}

}
}
