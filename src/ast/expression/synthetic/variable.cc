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
  if (explicitType) o << " " << explicitType->stringify();
  o << ")";
}

VariableNode*
VariableNodeSynthetic::cloneImpl() {
  return new VariableNodeSynthetic(name, explicitType);
}

VariableNode*
VariableNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  assert(!explicitType ||
         (t.translate(
              getSymbolTable()->getSemanticContext(), explicitType)
            ->equals(*explicitType)));
  return new VariableNodeSynthetic(name, explicitType);
}

}
}
