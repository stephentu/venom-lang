#include <ast/expression/intliteral.h>

#include <analysis/type.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
IntLiteralNode::typeCheck(SemanticContext*  ctx,
                          InstantiatedType* expected) {
  return InstantiatedType::IntType;
}

}
}
