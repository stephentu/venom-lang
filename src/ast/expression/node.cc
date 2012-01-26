#include <algorithm>
#include <sstream>

#include <analysis/semanticcontext.h>

#include <ast/expression/functioncall.h>
#include <ast/expression/node.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

struct stringer_functor {
  inline string operator()(const ParameterizedTypeString* t) const {
    return t->stringify();
  }
} stringer;

string ParameterizedTypeString::stringify() const {
  stringstream s;
  s << util::join(names.begin(), names.end(), ".");
  if (!params.empty()) {
    s << "{";
    vector<string> buf;
    buf.resize(params.size());
    transform(params.begin(), params.end(), buf.begin(), stringer);
    s << util::join(buf.begin(), buf.end(), ",");
    s << "}";
  }
  return s.str();
}

ParameterizedTypeString* ParameterizedTypeString::clone() {
  return new ParameterizedTypeString(names,
      util::transform_vec(params.begin(), params.end(), CloneFunctor()));
}

void
ASTExpressionNode::collectInstantiatedTypes(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  ASTNode::collectInstantiatedTypes(ctx, t, callback);
  if (staticType) {
    InstantiatedType* itype = t.translate(ctx, staticType);
    if (itype->isSpecializedType()) callback.offer(itype);
  }
}

ASTNode*
ASTExpressionNode::rewriteLocal(SemanticContext* ctx,
                                RewriteMode mode) {
  if (mode != BoxPrimitives) return ASTNode::rewriteLocal(ctx, mode);

  // recurse on children
  ASTNode* rep = ASTNode::rewriteLocal(ctx, mode);
  VENOM_ASSERT_NULL(rep);

  // if the expected type is any, and the
  // static type is a primitive, then we need to box
  assert(getStaticType());

  if (getExpectedType() &&
      getExpectedType()->isAny() &&
      getStaticType()->isPrimitive()) {
    ClassSymbol* boxClass = NULL;
    if (getStaticType()->isInt()) {
      boxClass = Type::BoxedIntType->getClassSymbol();
    } else if (getStaticType()->isFloat()) {
      boxClass = Type::BoxedFloatType->getClassSymbol();
    } else if (getStaticType()->isBool()) {
      boxClass = Type::BoxedBoolType->getClassSymbol();
    } else assert(false);
    assert(boxClass);

    FunctionCallNode* rep =
      new FunctionCallNodeSynthetic(
          new SymbolNode(boxClass),
          InstantiatedTypeVec(),
          util::vec1(this->clone(CloneMode::Semantic)));
    return replace(ctx, rep);
  }
  return NULL;
}

ASTExpressionNode*
ASTExpressionNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, replacement);
  return static_cast<ASTExpressionNode*>(ASTNode::replace(ctx, replacement));
}

}
}
