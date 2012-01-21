#include <ast/expression/nilliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
NilLiteralNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  if (expected && !expected->isPrimitive()) {
    return expected;
  }
  return InstantiatedType::AnyType;
}

void
NilLiteralNode::codeGen(CodeGenerator& cg) {
  cg.emitInst(Instruction::PUSH_CELL_NIL);
}

NilLiteralNode*
NilLiteralNode::cloneImpl() {
  return new NilLiteralNode;
}

ASTExpressionNode*
NilLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new NilLiteralNode;
}

NilLiteralNode*
NilLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new NilLiteralNode;
}

}
}
