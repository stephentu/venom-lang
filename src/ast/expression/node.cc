#include <algorithm>
#include <sstream>

#include <analysis/semanticcontext.h>
#include <ast/expression/node.h>
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
ASTExpressionNode::cloneSetState(ASTNode* node) {
  VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, node);
  ASTExpressionNode* enode = static_cast<ASTExpressionNode*>(node);

  ASTNode::cloneSetState(node);

  enode->staticType   = staticType;
  enode->expectedType = expectedType;
  enode->typeParams   = typeParams;
}

ASTExpressionNode*
ASTExpressionNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, replacement);
  return static_cast<ASTExpressionNode*>(ASTNode::replace(ctx, replacement));
}

}
}
