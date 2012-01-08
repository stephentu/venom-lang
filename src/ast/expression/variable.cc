#include <ast/expression/attraccess.h>
#include <ast/expression/symbolnode.h>
#include <ast/expression/variable.h>
#include <ast/statement/classdecl.h>

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

BaseSymbol*
VariableNode::getSymbol() {
  TypeTranslator t;
  return symbols->findBaseSymbol(
      name, SymbolTable::Any, SymbolTable::AllowCurrentScope, t);
}

void
VariableNode::registerSymbol(SemanticContext* ctx) {
  if (!symbols->isDefined(
        name, SymbolTable::Any, SymbolTable::AllowCurrentScope)) {
    throw SemanticViolationException(
        "Symbol " + name + " is not defined in scope");
  }
}

InstantiatedType*
VariableNode::typeCheckImpl(SemanticContext* ctx,
                            InstantiatedType* expected,
                            const InstantiatedTypeVec& typeParamArgs) {
  TypeTranslator t;
  BaseSymbol *sym =
    symbols->findBaseSymbol(
        name, SymbolTable::Any, SymbolTable::AllowCurrentScope, t);
  assert(sym);
  return sym->bind(ctx, t, typeParamArgs);
}

ASTNode*
VariableNode::rewriteLocal(SemanticContext* ctx) {
  BaseSymbol *bs = getSymbol();
  assert(bs);
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    if (sym->isModuleLevelSymbol()) {
      // need to rewrite x to reference x off of the
      // module object
      ModuleSymbol *msym =
        ctx->getRootSymbolTable()->findModuleSymbol(
            ctx->getModuleName(), SymbolTable::NoRecurse);
      assert(msym);
      AttrAccessNode *rep =
        new AttrAccessNode(
            new SymbolNode(msym, msym->getModuleType()->instantiate(ctx)),
            name);
      return replace(ctx, rep);
    }
  }
  return replace(ctx, createSymbolNode());
}

void
VariableNode::codeGen(CodeGenerator& cg) {
  BaseSymbol *bs = getSymbol();
  assert(bs);
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    size_t idx = cg.getLocalVariable(sym);
    cg.emitInstU32(Instruction::LOAD_LOCAL_VAR, idx);
  } else {
    // otherwise if we are referencing a module/class,
    // then wait until the AttrAccess node to generate
    // code
  }
}

ASTExpressionNode*
VariableNode::createSymbolNode() {
  BaseSymbol *bs = getSymbol();
  assert(bs);
  assert(staticType);
  return new SymbolNode(bs, staticType);
}

void
VariableSelfNode::registerSymbol(SemanticContext* ctx) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  if (!cdn) {
    throw SemanticViolationException(
        "self cannot be used outside of class scope");
  }
}

InstantiatedType*
VariableSelfNode::typeCheckImpl(SemanticContext* ctx,
                                InstantiatedType* expected,
                                const InstantiatedTypeVec& typeParamArgs) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  assert(cdn);
  TypeTranslator t;
  ClassSymbol *cs =
    cdn->getSymbolTable()->findClassSymbol(
        cdn->getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getSelfType(ctx);
}

void
VariableSelfNode::codeGen(CodeGenerator& cg) {
  VENOM_UNIMPLEMENTED;
}

void
VariableSuperNode::registerSymbol(SemanticContext* ctx) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  if (!cdn) {
    throw SemanticViolationException(
        "super cannot be used outside of class scope");
  }
}

InstantiatedType*
VariableSuperNode::typeCheckImpl(SemanticContext* ctx,
                                 InstantiatedType* expected,
                                 const InstantiatedTypeVec& typeParamArgs) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  assert(cdn);
  TypeTranslator t;
  ClassSymbol *cs =
    cdn->getSymbolTable()->findClassSymbol(
        cdn->getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getType()->getParent();
}

void
VariableSuperNode::codeGen(CodeGenerator& cg) {
  VENOM_UNIMPLEMENTED;
}

}
}
