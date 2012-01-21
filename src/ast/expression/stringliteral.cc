#include <ast/expression/stringliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
StringLiteralNode::typeCheckImpl(SemanticContext* ctx,
                                 InstantiatedType* expected,
                                 const InstantiatedTypeVec& typeParamArgs) {
  return InstantiatedType::StringType;
}

void
StringLiteralNode::codeGen(CodeGenerator& cg) {
  bool create;
  size_t idx = cg.createConstant(Constant(value), create);
  cg.emitInstU32(Instruction::PUSH_CONST, idx);
}

StringLiteralNode*
StringLiteralNode::cloneImpl() {
  return new StringLiteralNode(value);
}

ASTExpressionNode*
StringLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new StringLiteralNode(value);
}

StringLiteralNode*
StringLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new StringLiteralNode(value);
}

}
}
