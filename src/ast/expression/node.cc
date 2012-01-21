#include <algorithm>
#include <sstream>

#include <analysis/semanticcontext.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/node.h>
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
ASTExpressionNode::collectInstantiatedTypes(vector<InstantiatedType*>& types) {
  ASTNode::collectInstantiatedTypes(types);
  if (staticType && staticType->isSpecializedType()) {
    types.push_back(staticType);
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

    TypeTranslator t;
    FunctionCallNode* rep =
      new FunctionCallNodeParser(
          new SymbolNode(
            boxClass,
            boxClass->bind(ctx, t, InstantiatedTypeVec()),
            NULL),
          TypeStringVec(),
          util::vec1(this->clone()));
    return replace(ctx, rep);
  }
  return NULL;
}

void
ASTExpressionNode::cloneSetState(ASTNode* node) {
  ASTNode::cloneSetState(node);

  //VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, node);
  //ASTExpressionNode* enode = static_cast<ASTExpressionNode*>(node);

  //ASTNode::cloneSetState(node);

  //enode->staticType   = staticType;
  //enode->expectedType = expectedType;
  //enode->typeParams   = typeParams;
}

ASTExpressionNode*
ASTExpressionNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, replacement);
  return static_cast<ASTExpressionNode*>(ASTNode::replace(ctx, replacement));
}

}
}
