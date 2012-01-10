#include <ast/expression/stringliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

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
  size_t idx = cg.createConstant(value);
  cg.emitInstU32(Instruction::PUSH_CONST, idx);
}

StringLiteralNode*
StringLiteralNode::cloneImpl() {
  return new StringLiteralNode(value);
}

}
}
