#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayaccess.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
ArrayAccessNode::typeCheck(SemanticContext*  ctx,
                           InstantiatedType* expected) {
  InstantiatedType *indexType = index->typeCheck(ctx, NULL);
  if (!indexType->equals(*InstantiatedType::IntType)) {
    throw TypeViolationException(
        "Invalid index type: " + indexType->stringify());
  }
  InstantiatedType *primaryType = primary->typeCheck(ctx, NULL);
  if (!indexType->getType()->equals(*Type::ListType)) {
    throw TypeViolationException(
        "Cannot subscript non-list type: " + primaryType->stringify());
  }
  return primaryType->getParams().at(0);
}

}
}
