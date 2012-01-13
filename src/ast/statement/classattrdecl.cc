#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>
#include <ast/statement/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassAttrDeclNode::registerSymbol(SemanticContext* ctx) {
  assert(hasLocationContext(TopLevelClassBody));
  VENOM_ASSERT_TYPEOF_PTR(VariableNode, variable);
  VariableNode *var = static_cast<VariableNode*>(variable);

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

  VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, symbols->getOwner());
  ClassDeclNode *cdn = static_cast<ClassDeclNode*>(symbols->getOwner());
  BaseSymbol *classSymbol = cdn->getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, classSymbol);

  symbols->createClassAttributeSymbol(
      var->getName(), itype, static_cast<ClassSymbol*>(classSymbol));
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
  BaseSymbol *bs = variable->getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassAttributeSymbol, bs);
  ClassAttributeSymbol *sym = static_cast<ClassAttributeSymbol*>(bs);
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
    value = sym->getInstantiatedType()->getType()->createDefaultInitializer();
    value->initSymbolTable(symbols);
    // no need to call semantic check on value
  }

  AssignNode::TypeCheckAssignment(
      ctx, symbols, variable, value, sym->getClassSymbol());
}

ClassAttrDeclNode*
ClassAttrDeclNode::cloneImpl() {
  return new ClassAttrDeclNode(
      variable->clone(),
      value ? value->clone() : NULL);
}

}
}
