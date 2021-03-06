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

#include <ast/expression/arrayaccess.h>
#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <ast/statement/assign.h>
#include <ast/statement/stmtexpr.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
AssignNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  if (var) {
    RegisterVariableLHS(ctx, symbols, var, this);
  }
}

ASTNode*
AssignNode::rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const set<BaseSymbol*>& refs) {
  BaseSymbol* psym = variable->getSymbol();
  if (psym) {
    set<BaseSymbol*>::const_iterator it = refs.find(psym);
    if (it != refs.end()) {
      VENOM_ASSERT_TYPEOF_PTR(Symbol, psym);
      Symbol* sym = static_cast<Symbol*>(psym);
      if (sym->getDecl() == this) {

        // WARNING: this has to be done carefully so we don't
        // generate any type errors. The order of operations
        // done here is very important for correctness

        VENOM_ASSERT_TYPEOF_PTR(VariableNode, variable);

        ExprListNode* exprs = new ExprListNode;
        StmtExprNode* stmtexpr = new StmtExprNode(exprs);

        SemanticContext* ctx = symbols->getSemanticContext();

        // this will replace this assignment. but first, clone it
        // and semantic/type check it (rewrite below)
        AssignExprNode* assignExpr =
            new AssignExprNode(
                variable->clone(CloneMode::Semantic),
                value->clone(CloneMode::Semantic));
        assignExpr->initSymbolTable(symbols);
        assignExpr->semanticCheck(ctx);
        assignExpr->typeCheck(ctx);

        assert(sym->getDefinedSymbolTable() == symbols);
        // need to mark this symbol ref-ed, so type checking
        // doesn't fail...
        sym->markPromoteToRef();

        // need to add an instantiation of the ref
        exprs->appendExpression(
          new AssignExprNode(
            variable->clone(CloneMode::Semantic),
            new FunctionCallNodeSynthetic(
              new VariableNodeParser(
                variable->getStaticType()->refify(ctx)->createClassName(),
                NULL),
              InstantiatedTypeVec(),
              ExprNodeVec())));

        // semantic/typecheck so that we can rewrite the var/value
        // w/o type errors
        replace(ctx, stmtexpr);

        // now it is safe to do a rewrite on assignExpr
        ASTNode* retVal = assignExpr->rewriteAfterLift(liftMap, refs);
        VENOM_ASSERT_NULL(retVal);

        // append does not do another typecheck
        exprs->appendExpression(assignExpr);

        return stmtexpr;
      }
    }
  }
  return ASTStatementNode::rewriteAfterLift(liftMap, refs);
}

void
AssignNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // TODO: re-factor code duplication between here and
  // ast/expression/assignexpr.cc

  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);

  // register now, before recursing on variable
  if (doRegister) registerSymbol(ctx);

  // now recurse on variable
  variable->semanticCheckImpl(ctx, true);
}

void
AssignNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  assert(value);
  decl_either decl(this);
  TypeCheckAssignment(ctx, symbols, variable, value, decl);
  checkExpectedType(expected);
}

void
AssignNode::codeGen(CodeGenerator& cg) {
  CodeGenAssignment(cg, variable, value);
}

AssignNode*
AssignNode::cloneImpl(CloneMode::Type type) {
  return new AssignNode(variable->clone(type), value->clone(type));
}

ASTStatementNode*
AssignNode::cloneForLiftImpl(LiftContext& ctx) {
  return new AssignNode(variable->cloneForLift(ctx), value->cloneForLift(ctx));
}

AssignNode*
AssignNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AssignNode(
      variable->cloneForTemplate(t), value->cloneForTemplate(t));
}

InstantiatedType*
AssignNode::TypeCheckAssignment(
    SemanticContext* ctx,
    SymbolTable* symbols,
    ASTExpressionNode* variable,
    ASTExpressionNode* value,
    decl_either& decl) {
  InstantiatedType *lhs = variable->typeCheck(ctx, NULL);
  InstantiatedType *rhs = value->typeCheck(ctx, lhs);
  assert(rhs);
  if (!rhs->getType()->isVisible()) {
    throw TypeViolationException(
        "Cannot create reference to hidden type " + rhs->stringify());
  }

  if (lhs) {
    // require rhs <: lhs
    if (!rhs->isSubtypeOf(*lhs) &&
        !rhs->getClassSymbol()->isLiftOf(lhs->getClassSymbol()) &&
        !rhs->isSpecializationOf(lhs)) {
      throw TypeViolationException(
          "Cannot assign type " + rhs->stringify() + " to type " + lhs->stringify());
    }
    return lhs;
  } else {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, variable);
    VariableNode *vn = static_cast<VariableNode*>(variable);
    assert(!vn->getExpectedType());
    BaseSymbol* bs = vn->getSymbol();
    VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
    Symbol* sym = static_cast<Symbol*>(bs);

    // some sanity assertions
    if (decl.isRight()) {
      VENOM_ASSERT_TYPEOF_PTR(ClassAttributeSymbol, sym);
      assert(static_cast<ClassAttributeSymbol*>(sym)->getClassSymbol() ==
             decl.right());
    } else {
      assert(sym->getDecl() == decl.left());
    }

    sym->setInstantiatedType(rhs);

    // go again, so we can set the static type on variable
    return TypeCheckAssignment(ctx, symbols, variable, value, decl);
  }
}

void
AssignNode::RegisterVariableLHS(SemanticContext* ctx,
                                SymbolTable* symbols,
                                VariableNode* var,
                                ASTNode* decl) {
  // check for duplicate definition (as a function or class)
  if (symbols->isDefined(
        var->getName(), SymbolTable::Function | SymbolTable::Class,
        SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Symbol " + var->getName() + " already defined");
  }

  InstantiatedType* explicitType = var->getExplicitType();
  if (explicitType) {
    // if there is an explicit type string, treat it as
    // explicitly declaring a new symbol
    if (symbols->isDefined(
          var->getName(), SymbolTable::Location, SymbolTable::NoRecurse)) {
      throw SemanticViolationException(
          "Cannot redeclare symbol " + var->getName());
    }
    symbols->createSymbol(var->getName(), explicitType, decl);
  } else {
    // if there is no type string, then only create a new
    // declaration if the symbol doesn't exist anywhere in the scope
    // (current or parents) as a location type
    //
    // for example, consider case 1:
    // class A
    //   attr x::int
    // end
    // class B <- A
    //   def self(y::int) =
    //     x = y; # does *NOT* declare a new symbol
    //   end
    // end
    //
    // versus:
    //
    // class A
    //   def x() = print('hi'); end
    // end
    // class B <- A
    //   def self(y::int) =
    //     x = y; # *does* declare a new symbol
    //   end
    // end
    if (!symbols->isDefined(
          var->getName(), SymbolTable::Location,
          SymbolTable::AllowCurrentScope)) {
      symbols->createSymbol(var->getName(), NULL, decl);
    }
  }
}

void
AssignNode::CodeGenAssignment(CodeGenerator& cg,
                              ASTExpressionNode* variable,
                              ASTExpressionNode* value) {

  // case 1:
  // exprA[exprB] = exprC
  if (ArrayAccessNode *aan = dynamic_cast<ArrayAccessNode*>(variable)) {
    aan->codeGenAssignLHS(cg, value);
    return;
  }

  // case 2:
  // exprA.field = exprB

  // case 3:
  // location = exprA

  // TODO: we need to check these conditions actually
  // hold (during static analysis)
  BaseSymbol* bs = variable->getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
  Symbol* sym = static_cast<Symbol*>(bs);

  bool refCnt = variable->getStaticType()->isRefCounted();
  if (sym->isModuleLevelSymbol() || sym->isObjectField()) {
    variable->codeGen(cg);
    value->codeGen(cg);
    size_t slotIdx = sym->getFieldIndex();
    cg.emitInstU32(
        refCnt ?
          Instruction::SET_ATTR_OBJ_REF :
          Instruction::SET_ATTR_OBJ,
        slotIdx);
  } else {
    value->codeGen(cg);
    // no codegen for variable, since we can just store directly
    // into the local variable array
    bool create;
    size_t idx = cg.createLocalVariable(sym, create);
    cg.emitInstU32(
        refCnt ?
          Instruction::STORE_LOCAL_VAR_REF :
          Instruction::STORE_LOCAL_VAR,
        idx);
  }
}

}
}
