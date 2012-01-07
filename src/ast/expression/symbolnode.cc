#include <ast/expression/symbolnode.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
SymbolNode::typeCheckImpl(SemanticContext* ctx,
                          InstantiatedType* expected,
                          const InstantiatedTypeVec& typeParamArgs) {
  TypeTranslator t;
  return symbol->bind(ctx, t, typeParamArgs);
}

void
SymbolNode::print(ostream& o, size_t indent) {
  o << "(synthetic-symbol-node " << symbol->getFullName() << ")";
}

}
}
