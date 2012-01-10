#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassAttrDeclNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  assert(var);

  // don't allow an attr to overshadow any decl
  // in a parent
  if (symbols->isDefined(
        var->getName(), SymbolTable::Any, SymbolTable::ClassParents)) {
    throw SemanticViolationException(
        "Name " + var->getName() + " already defined in parent");
  }

  if (symbols->isDefined(
        var->getName(), SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Name " + var->getName() + " already defined in class");
  }

  InstantiatedType *itype = NULL;
  if (var->getExplicitParameterizedTypeString()) {
    itype = ctx->instantiateOrThrow(
        symbols, var->getExplicitParameterizedTypeString());
  }

  symbols->createSymbol(var->getName(), true, itype);
}

void
ClassAttrDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
}

void
ClassAttrDeclNode::typeCheck(SemanticContext* ctx,
                             InstantiatedType* expected) {
  assert(!expected);
  if (!value) {
    // replace
    //   attr x::T
    // with
    //   attr x::T = <default initializer>
    //
    // where <default initializer> is:
    //   0 (T = Int)
    //   0.0 (T = Double)
    //   False (T = Bool)
    //   Nil (otherwise)

    BaseSymbol *bs = variable->getSymbol();
    assert(bs);
    assert(dynamic_cast<Symbol*>(bs));
    Symbol *sym = static_cast<Symbol*>(bs);
    value = sym->getInstantiatedType()->getType()->createDefaultInitializer();
    value->initSymbolTable(symbols);
    // no need to call semantic check on value
  }

  AssignNode::TypeCheckAssignment(ctx, symbols, variable, value, true);
}

ClassAttrDeclNode*
ClassAttrDeclNode::cloneImpl() {
  return new ClassAttrDeclNode(
      variable->clone(),
      value ? value->clone() : NULL);
}

}
}
