#include <cassert>
#include <utility>

#include <ast/expression/dictliteral.h>
#include <analysis/type.h>
#include <backend/codegenerator.h>
#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

typedef pair<InstantiatedType*, InstantiatedType*> ITypePair;

struct fold_functor {
  fold_functor(SemanticContext* ctx) : ctx(ctx) {}
  inline ITypePair
  operator()(const ITypePair& accum, DictPair* cur) const {
    if (!accum.first) {
      assert(!accum.second);
      return ITypePair(
          cur->key()->typeCheck(ctx),
          cur->value()->typeCheck(ctx));
    } else {
      assert(accum.second);
      return ITypePair(
          accum.first->mostCommonType(cur->key()->typeCheck(ctx)),
          accum.second->mostCommonType(cur->value()->typeCheck(ctx)));
    }
  }
  SemanticContext* ctx;
};

DictPair*
DictPair::cloneImpl() {
  return new DictPair(first->clone(), second->clone());
}

InstantiatedType*
DictLiteralNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {
  if (pairs.empty()) {
    if (expected && expected->getType()->equals(*Type::MapType)) {
      return expected;
    }
    return Type::MapType->instantiate(
        ctx, util::vec2(InstantiatedType::AnyType, InstantiatedType::AnyType));
  } else {
    ITypePair res =
      util::foldl(pairs.begin(), pairs.end(),
                  ITypePair(NULL, NULL), fold_functor(ctx));
    return Type::MapType->instantiate(
        ctx, util::vec2(res.first, res.second));
  }
}

DictLiteralNode*
DictLiteralNode::cloneImpl() {
  return new DictLiteralNode(
      util::transform_vec(
        pairs.begin(), pairs.end(), DictPair::CloneFunctor()));
}

}
}
