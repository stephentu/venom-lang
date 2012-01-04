#include <ast/expression/nilliteral.h>

#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
NilLiteralNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  if (expected && expected->isSubtypeOf(*InstantiatedType::ObjectType)) {
    return expected;
  }
  return InstantiatedType::ObjectType;
}

void
NilLiteralNode::codeGen(CodeGenerator& cg) {
  cg.emitInst(Instruction::PUSH_CELL_NIL);
}

}
}
