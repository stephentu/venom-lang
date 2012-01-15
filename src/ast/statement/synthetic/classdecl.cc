#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/synthetic/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassDeclNodeSynthetic::registerSymbol(SemanticContext* ctx) {
  // check to see if this class is already defined in this scope
  if (symbols->isDefined(name, SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Class " + name + " already defined");
  }

  if (parentTypes.size() > 1) {
    throw SemanticViolationException(
        "Multiple inheritance currently not supported");
  }

  for (size_t pos = 0; pos < typeParamTypes.size(); pos++) {
    // add all the type params into the body's symtab
    VENOM_ASSERT_TYPEOF_PTR(TypeParamType, typeParamTypes[pos]->getType());
    stmts->getSymbolTable()->createClassSymbol(
        typeParamTypes[pos]->getType()->getName(),
        ctx->getRootSymbolTable()->newChildScope(NULL),
        typeParamTypes[pos]->getType());
  }

  registerClassSymbol(ctx, parentTypes, typeParamTypes);
}

void
ClassDeclNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(class " << name << std::endl << util::indent(indent + 1);
  vector<string> typenames(typeParamTypes.size());
  transform(typeParamTypes.begin(), typeParamTypes.end(),
            typenames.begin(),
            util::stringify_functor<InstantiatedType>::ptr());
  o << "(type-params (" <<
    util::join(typenames.begin(), typenames.end(), ",") <<
    "))" << std::endl << util::indent(indent + 1);
  stmts->print(o, indent + 1);
  o << ")";
}

ClassDeclNodeSynthetic*
ClassDeclNodeSynthetic::cloneImpl() {
  return new ClassDeclNodeSynthetic(
      name,
      parentTypes,
      typeParamTypes,
      stmts);
}

}
}
