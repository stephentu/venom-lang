#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayliteral.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

struct functor {
  functor(SemanticContext* ctx) : ctx(ctx) {}
  inline InstantiatedType* operator()(ASTExpressionNode* exp) const {
    return exp->typeCheck(ctx, NULL);
  }
  SemanticContext* ctx;
};

struct reduce_functor_t {
  inline InstantiatedType*
  operator()(InstantiatedType* accum, InstantiatedType* cur) const {
    return accum->mostCommonType(cur);
  }
} reduce_functor;

InstantiatedType*
ArrayLiteralNode::typeCheck(SemanticContext*  ctx,
                            InstantiatedType* expected) {
  if (values.empty()) {
    if (expected && expected->getType()->equals(*Type::ListType)) {
      return expected;
    }
    return Type::ListType->instantiate(
        ctx, util::vec1(InstantiatedType::AnyType));
  } else {
    vector<InstantiatedType*> types(values.size());
    transform(values.begin(), values.end(), types.begin(),
              functor(ctx));
    return Type::ListType->instantiate(
        ctx, util::vec1(
          util::reducel(types.begin(), types.end(), reduce_functor)));
  }
}

}
}
