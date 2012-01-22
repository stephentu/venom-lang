#include <ast/expression/synthetic/variable.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
VariableNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(ident " << name;
  o << " " << explicitType->stringify();
	if (symbol) o << " (sym-addr " << std::hex << symbol << ")";
  o << ")";
}

VariableNode*
VariableNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new VariableNodeSynthetic(name, explicitType);
}

ASTExpressionNode*
VariableNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {

  // assert that we should never be a non-local ref
#ifndef NDEBUG
  Symbol* s;
  assert(!isNonLocalRef(ctx.definedIn, s));
  BaseSymbol* bs = symbol;
  assert(bs != ctx.curLiftSym);
  LiftContext::LiftMap::const_iterator it =
    ctx.liftMap.find(bs);
  assert(it == ctx.liftMap.end());
#endif /* NDEBUG */

  return new VariableNodeSynthetic(name, explicitType);
}

VariableNode*
VariableNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  assert((t.translate(
              getSymbolTable()->getSemanticContext(), explicitType)
            ->equals(*explicitType)));
  return new VariableNodeSynthetic(name, explicitType);
}

}
}
