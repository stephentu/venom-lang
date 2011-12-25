#include <ast/expression/stringliteral.h>

#include <analysis/type.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
StringLiteralNode::typeCheck(SemanticContext*  ctx,
                             InstantiatedType* expected) {
  return InstantiatedType::StringType;
}

}
}
