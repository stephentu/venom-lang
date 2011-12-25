#include <cassert>
#include <utility>

#include <ast/expression/dictliteral.h>
#include <analysis/type.h>
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
          cur->key()->typeCheck(ctx, NULL),
          cur->value()->typeCheck(ctx, NULL));
    } else {
      assert(accum.second);
      return ITypePair(
          accum.first->mostCommonType(cur->key()->typeCheck(ctx, NULL)),
          accum.second->mostCommonType(cur->value()->typeCheck(ctx, NULL)));
    }
  }
  SemanticContext* ctx;
};

InstantiatedType*
DictLiteralNode::typeCheck(SemanticContext*  ctx,
                           InstantiatedType* expected) {
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

}
}