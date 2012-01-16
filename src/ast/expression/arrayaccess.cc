#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayaccess.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
ArrayAccessNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {

  InstantiatedType *primaryType = primary->typeCheck(ctx);
  InstantiatedType *indexType = index->typeCheck(ctx);

  bool isList = primaryType->getType()->equals(*Type::ListType);
  bool isMap  = primaryType->getType()->equals(*Type::MapType);
  bool isStr  = primaryType->getType()->equals(*Type::StringType);

  if (!isList && !isMap && !isStr) {
    throw TypeViolationException(
        "Cannot subscript non-list/non-map/non-string type: " +
        primaryType->stringify());
  }

  if (isList || isStr) {
    if (!indexType->equals(*InstantiatedType::IntType)) {
      throw TypeViolationException(
          "Invalid index type - expecting int, got " +
          indexType->stringify());
    }
    return isList ? primaryType->getParams().at(0) : primaryType;
  } else {
    InstantiatedType *keyType = primaryType->getParams().at(0);
    if (!indexType->equals(*keyType)) {
      throw TypeViolationException(
          "Invalid index type - expecting " + keyType->stringify() +
          ", got " + indexType->stringify());
    }
    return primaryType->getParams().at(1);
  }
}

ArrayAccessNode*
ArrayAccessNode::cloneImpl() {
  return new ArrayAccessNode(primary->clone(), index->clone());
}

ArrayAccessNode*
ArrayAccessNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ArrayAccessNode(
      primary->cloneForTemplate(t), index->cloneForTemplate(t));
}

}
}
