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
VariableNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  if (mode != CanonicalRefs) return ASTNode::rewriteLocal(ctx, mode);

  BaseSymbol *bs = getSymbol();
  assert(bs);
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    if (sym->isModuleLevelSymbol()) {
      assert(!sym->isObjectField());

      // need to rewrite x to reference x off of the
      // module object
      ModuleSymbol *msym =
        ctx->getRootSymbolTable()->findModuleSymbol(
            ctx->getModuleName(), SymbolTable::NoRecurse);
      assert(msym);
      AttrAccessNode *rep =
        new AttrAccessNode(
            new SymbolNode(
              msym,
              msym->getModuleClassSymbol()->getType()->instantiate(ctx),
              NULL),
            name);
      return replace(ctx, rep);
    }

    if (sym->isObjectField()) {
      // rewrite x into self.x
      AttrAccessNode *rep = new AttrAccessNode(new VariableSelfNode, name);
      return replace(ctx, rep);
    }
  }
  //return replace(ctx, createSymbolNode());
  return NULL;
}

void
VariableNode::codeGen(CodeGenerator& cg) {
  BaseSymbol *bs = getSymbol();
  assert(bs);
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    bool create;
    size_t idx = cg.createLocalVariable(sym, create);
    assert(!create);
    cg.emitInstU32(
        sym->getInstantiatedType()->isPrimitive() ?
          Instruction::LOAD_LOCAL_VAR : Instruction::LOAD_LOCAL_VAR_REF,
        idx);
  } else {
    // otherwise if we are referencing a module/class,
    // then wait until the AttrAccess node to generate
    // code
  }
}

VariableNode*
VariableNode::cloneImpl() {
  return new VariableNode(
      name, explicit_type ? explicit_type->clone() : NULL);
}

ASTExpressionNode*
VariableNode::createSymbolNode() {
  BaseSymbol *bs = getSymbol();
  assert(bs);
  assert(staticType);
  return new SymbolNode(bs, staticType, expectedType);
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
  // self is always slot 0
  cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, 0);
}

VariableSelfNode*
VariableSelfNode::cloneImpl() {
  return new VariableSelfNode;
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
  // super (self) is always slot 0
  cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, 0);
}

VariableSuperNode*
VariableSuperNode::cloneImpl() {
  return new VariableSuperNode;
}

}
}
