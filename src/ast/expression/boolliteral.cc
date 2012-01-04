#include <ast/expression/boolliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
BoolLiteralNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {
  return InstantiatedType::BoolType;
}

void
BoolLiteralNode::codeGen(CodeGenerator& cg) {
  cg.emitInstBool(Instruction::PUSH_CELL_BOOL, value);
}

}
}
