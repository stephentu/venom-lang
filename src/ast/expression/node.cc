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

ASTExpressionNode*
ASTExpressionNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  assert(dynamic_cast<ASTExpressionNode*>(replacement));
  return static_cast<ASTExpressionNode*>(ASTNode::replace(ctx, replacement));
}

}
}
