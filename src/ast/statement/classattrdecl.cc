/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/variable.h>

#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>
#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtlist.h>

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
  if (mode != DeSugar) return NULL;

  if (value) {
    // need to copy initialization into ctor

    // find ctor stmt list
    ClassDeclNode* cdn = getEnclosingClassNode();
    assert(cdn);
    ClassSymbol* csym = cdn->getClassSymbol();
    TypeTranslator t;
    FuncSymbol* ctorSym = csym->getClassSymbolTable()->findFuncSymbol(
        "<ctor>", SymbolTable::NoRecurse, t);
    assert(ctorSym);
    VENOM_ASSERT_TYPEOF_PTR(CtorDeclNode,
                            ctorSym->getFunctionSymbolTable()->getOwner());
    CtorDeclNode* ctorNode =
      static_cast<CtorDeclNode*>(ctorSym->getFunctionSymbolTable()->getOwner());
    StmtListNode* ctorStmts = ctorNode->getStmts();

    // insert self.var = value into ctor stmts
    ASTStatementNode* assign =
      new AssignNode(
          new AttrAccessNode(
            new VariableSelfNode,
            static_cast<VariableNode*>(variable)->getName()),
          value->clone(CloneMode::Structural));

    SemanticContext* ctx = ctorStmts->getSymbolTable()->getSemanticContext();
    assign->initSymbolTable(ctorStmts->getSymbolTable());
    assign->semanticCheck(ctx);
    assign->typeCheck(ctx);

    ctorStmts->insertStatement(1, assign);
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
