#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>
#include <ast/statement/classdecl.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

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

  InstantiatedType *itype = var->getExplicitType();

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
  if (doRegister) registerSymbol(ctx);
	variable->semanticCheckImpl(ctx, true);
}

void
ClassAttrDeclNode::typeCheck(SemanticContext* ctx,
                             InstantiatedType* expected) {
  assert(!expected);
  BaseSymbol *bs = variable->getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassAttributeSymbol, bs);
  ClassAttributeSymbol *sym = static_cast<ClassAttributeSymbol*>(bs);
  ASTExpressionNode* dummyValue = NULL;
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
    dummyValue =
			sym->getInstantiatedType()->getType()->createDefaultInitializer();
    //dummyValue->initSymbolTable(symbols);
    // no need to call semantic check on value
  }
  assert(value || dummyValue);

  AssignNode::decl_either decl(sym->getClassSymbol());
  AssignNode::TypeCheckAssignment(
      ctx, symbols, variable, value ? value : dummyValue, decl);

  if (dummyValue) delete dummyValue;
}

ASTNode*
ClassAttrDeclNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  for (size_t i = 1; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    if (!kid) continue;
    ASTNode* rep = kid->rewriteLocal(ctx, mode);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete kid;
    }
  }
  return NULL;
}

void
ClassAttrDeclNode::codeGen(CodeGenerator& cg) {
  // no-op - the assignments should have been copied into the
  // ctor
}

ClassAttrDeclNode*
ClassAttrDeclNode::cloneImpl(CloneMode::Type type) {
  return new ClassAttrDeclNode(
      variable->clone(type),
      value ? value->clone(type) : NULL);
}

ASTStatementNode*
ClassAttrDeclNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ClassAttrDeclNode(
      variable->cloneForLift(ctx),
      value ? value->cloneForLift(ctx) : NULL);
}

ClassAttrDeclNode*
ClassAttrDeclNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ClassAttrDeclNode(
      variable->cloneForTemplate(t),
      value ? value->cloneForTemplate(t) : NULL);
}

}
}
