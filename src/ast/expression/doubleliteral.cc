#include <ast/expression/doubleliteral.h>

#include <analysis/type.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
DoubleLiteralNode::typeCheck(SemanticContext*  ctx,
                             InstantiatedType* expected) {
  return InstantiatedType::FloatType;
}

}
}
