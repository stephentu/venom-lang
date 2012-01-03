#include <ast/expression/nilliteral.h>

#include <analysis/type.h>

using namespace venom::analysis;

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

}
}
