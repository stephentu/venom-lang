#include <ast/expression/boolliteral.h>

#include <analysis/type.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
BoolLiteralNode::typeCheck(SemanticContext*  ctx,
                           InstantiatedType* expected) {
  return InstantiatedType::BoolType;
}

}
}
