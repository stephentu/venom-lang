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

#include <algorithm>
#include <set>

#include <analysis/boundfunction.h>
#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/variable.h>

#include <ast/statement/assign.h>
#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>
#include <ast/statement/stmtexpr.h>
#include <ast/statement/stmtlist.h>

#include <ast/statement/synthetic/funcdecl.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;
using namespace venom::util;

namespace venom {
namespace ast {

struct name_functor_t {
  inline string operator()(ASTExpressionNode* node) const {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, node);
    return static_cast<VariableNode*>(node)->getName();
  }
} name_functor;

struct itype_functor {
  itype_functor(SemanticContext* ctx) : ctx(ctx) {}
  inline InstantiatedType* operator()(ASTExpressionNode* node) const {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, node);
    VariableNode *vn = static_cast<VariableNode*>(node);
    InstantiatedType* explicitType = vn->getExplicitType();
    assert(explicitType);
    return explicitType;
  }
  SemanticContext* ctx;
};

void FuncDeclNode::initSymbolTable(SymbolTable* symbols) {
  ASTStatementNode::initSymbolTable(symbols);
  // manually init symbol table of params, since the params aren't considered
  // to be children of this AST node
  for (ExprNodeVec::iterator it = params.begin();
       it != params.end(); ++it) {
    // must use the stmts symbol table
    (*it)->initSymbolTable(stmts->getSymbolTable());
  }
}

void FuncDeclNode::registerSymbol(SemanticContext* ctx) {
  // check current symbol to see if the symbol name is already taken
  if (symbols->isDefined(name, SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Name " + name + " already defined");
  }

  // don't allow a function to overshadow an attribute/class decl
  // in a parent
  if (symbols->isDefined(
        name, SymbolTable::Location | SymbolTable::Class,
        SymbolTable::ClassParents)) {
    throw SemanticViolationException(
        "Name " + name + " already defined in parent");
  }

  // check duplicate param names
  vector<string> names;
  names.resize(params.size());
  transform(params.begin(), params.end(), names.begin(), name_functor);
  if (!is_unique(names.begin(), names.end())) {
    throw SemanticViolationException("Duplicate parameter names");
  }

  // type params
  checkAndInitTypeParams(ctx);
  vector<InstantiatedType*> typeParamTypes = getTypeParams();

  // check and instantiate parameter types
  vector<InstantiatedType*> itypes(params.size());
  transform(params.begin(), params.end(),
            itypes.begin(), itype_functor(ctx));

  // check and instantiate return type
  checkAndInitReturnType(ctx);
  InstantiatedType* retType = getReturnType();

  if (hasLocationContext(TopLevelClassBody)) {
    VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, symbols->getOwner());
    ClassDeclNode *cdn = static_cast<ClassDeclNode*>(symbols->getOwner());
    VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, cdn->getSymbol());
    ClassSymbol *classSymbol = static_cast<ClassSymbol*>(cdn->getSymbol());

    if (isCtor()) {
      symbols->createMethodSymbol(name, stmts->getSymbolTable(),
                                  typeParamTypes, itypes,
                                  retType, classSymbol);
    } else {
      // check that type-signature matches for overrides
      TypeTranslator t;
      FuncSymbol *fs =
        symbols->findFuncSymbol(name, SymbolTable::ClassParents, t);
      if (fs) {
        assert(fs->isMethod());
        InstantiatedType *overrideType = fs->bind(ctx, t, typeParamTypes);

        vector<InstantiatedType*> fparams(itypes);
        fparams.push_back(retType);
        InstantiatedType *myType =
          Type::FuncTypes.at(itypes.size())->instantiate(ctx, fparams);

        if (!overrideType->equals(*myType)) {
          throw SemanticViolationException(
              "Overriding type signatures do not match: Cannot override method " +
              name + " of type " + overrideType->stringify() +
              " with type " + myType->stringify());
        }
      }
      symbols->createMethodSymbol(name, stmts->getSymbolTable(),
                                  typeParamTypes, itypes,
                                  retType, classSymbol, fs);
    }
  } else {
    symbols->createFuncSymbol(
        name, stmts->getSymbolTable(), typeParamTypes, itypes, retType);
  }

  // add parameters to block (child) symtab
  for (size_t i = 0; i < params.size(); i++) {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, params[i]);
    VariableNode *vn = static_cast<VariableNode*>(params[i]);
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), itypes[i], this);
  }
}

BaseSymbol*
FuncDeclNode::getSymbol() {
  TypeTranslator t;
  return symbols->findFuncSymbol(name, SymbolTable::NoRecurse, t);
}

void
FuncDeclNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  BaseSymbol *bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);
  FuncSymbol *fs = static_cast<FuncSymbol*>(bs);

  stmts->typeCheck(ctx, fs->getReturnType());
  checkExpectedType(expected);
}

void
FuncDeclNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {

  // parameters
  for (ExprNodeVec::iterator it = params.begin();
       it != params.end(); ++it) {
    (*it)->collectSpecialized(ctx, t, callback);
  }

  // ret type
  InstantiatedType* retType = t.translate(ctx, getReturnType());
  if (retType->isSpecializedType()) callback.offerType(retType);

  // stmts
  ASTNode::collectSpecialized(ctx, t, callback);
}

ASTNode*
FuncDeclNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  assert(!isTypeParameterized());

  // recurse on children first
  ASTNode *ret = ASTNode::rewriteLocal(ctx, mode);
  VENOM_ASSERT_NULL(ret);

  if (mode == FunctionReturns) {
    BaseSymbol *bs = getSymbol();
    VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);
    FuncSymbol *fs = static_cast<FuncSymbol*>(bs);
    if (fs->getReturnType()->equals(*InstantiatedType::VoidType)) {
      // if void return, just add a return void at the very end
      //
      // note: even if we have something like
      //   def foo () =
      //     if (...) then return; else return; end
      //   end
      // adding a return at the end does not alter the program semantics
      // and simplifies code generation
      ReturnNode *ret = new ReturnNode(NULL);
      ret->initSymbolTable(stmts->getSymbolTable());
      VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
      static_cast<StmtListNode*>(stmts)->appendStatement(ret);
    } else {
      // otherwise, we rewrite expr returns into explicit
      // returns, to simplify code generation
      ASTNode *ret = stmts->rewriteReturn(ctx);
      VENOM_ASSERT_NULL(ret);
    }
  }
  return NULL;
}

void
FuncDeclNode::codeGen(CodeGenerator& cg) {
  assert(!isTypeParameterized());

  BaseSymbol *bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);

  bool create;
  cg.enterLocalFunction(static_cast<FuncSymbol*>(bs), create);
  assert(create);
  cg.resetLocalVariables();

  // the calling convention is that args come in
  // the program stack like so:
  //                                  top -----
  //                                          ||
  //                                          \/
  // ret_addr | argN | argN-1 | ... | arg1 | arg0

  if (hasLocationContext(TopLevelClassBody)) {
    // must store the "this" pointer away first
    Symbol* temp = cg.createTemporaryVariable();
    bool create;
    size_t idx = cg.createLocalVariable(temp, create);
    assert(create);
    assert(idx == 0);
    cg.emitInstU32(Instruction::STORE_LOCAL_VAR_REF, idx);
  }

  for (ExprNodeVec::iterator it = params.begin();
       it != params.end(); ++it) {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, *it);
    VariableNode* vn = static_cast<VariableNode*>(*it);
    TypeTranslator t;
    Symbol *psym =
      stmts->getSymbolTable()->findSymbol(
          vn->getName(), SymbolTable::NoRecurse, t);
    assert(psym);
    // store symbol from stack into local variable slot
    bool create;
    size_t idx = cg.createLocalVariable(psym, create);
    assert(create);
    cg.emitInstU32(
        psym->getInstantiatedType()->isPrimitive() ?
          Instruction::STORE_LOCAL_VAR : Instruction::STORE_LOCAL_VAR_REF,
        idx);
  }
  stmts->codeGen(cg);
}

FuncDeclNode*
FuncDeclNode::newFuncDeclNodeForLift(
      LiftContext& ctx,
      const string& name,
      const ExprNodeVec& params,
      InstantiatedType* returnType,
      ASTStatementNode* stmts) {
  return new FuncDeclNodeParser(
      name,
      StrVec(),
      params,
      returnType->toParameterizedString(ctx.liftInto),
      stmts);
}

ASTStatementNode*
FuncDeclNode::cloneForLiftImplHelper(LiftContext& ctx) {
  assert(!isTypeParameterized());
  assert(ctx.isLiftingClass() || ctx.curLiftSym == getSymbol());

  // clone the stmts, then insert the lifted refs as
  // parameters to this function. also rename it.
  ASTStatementNode* stmtsClone = stmts->cloneForLift(ctx);
  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmtsClone);

  // ref params, only if we are lifting this function,
  // or this is the ctor of a class we are lifting
  ExprNodeVec newParams;
  if (ctx.isLiftingFunction() || isCtor())  {
    newParams.reserve(ctx.refs.vec.size() + params.size());
    for (vector<BaseSymbol*>::iterator it = ctx.refs.vec.begin();
         it != ctx.refs.vec.end(); ++it) {
      VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
      assert((*it)->getDefinedSymbolTable() == ctx.definedIn);
      Symbol* s = static_cast<Symbol*>(*it);
      newParams.push_back(
          new VariableNodeParser(
            ctx.refParamName(s),
            s->getInstantiatedType()
             ->findCodeGeneratableIType(ctx.definedIn->getSemanticContext())
             ->refify(ctx.definedIn->getSemanticContext())
             ->toParameterizedString(ctx.liftInto)));
      if (isCtor()) {
        // need to insert assignment statements into
        // the stmtlist. but must insert *after* the super ()
        // call
        static_cast<StmtListNode*>(stmtsClone)->insertStatement(
          1,
          new AssignNode(
            new AttrAccessNode(
              new VariableSelfNode, ctx.refParamName(s)),
            new VariableNodeParser(ctx.refParamName(s), NULL)));
      }
    }
  }

  // regular params
  TypeTranslator t;
  newParams.resize(newParams.size() + params.size());
  transform(params.begin(), params.end(),
            newParams.begin() + ctx.refs.vec.size(),
            ASTExpressionNode::CloneLiftFunctor(ctx));

  return newFuncDeclNodeForLift(
      ctx,
      ctx.isLiftingFunction() ? ctx.liftedName : name,
      newParams,
      getReturnType()
        ->findCodeGeneratableIType(symbols->getSemanticContext()),
      stmtsClone);
}

FuncDeclNode*
FuncDeclNode::cloneForTemplateImplHelper(const TypeTranslator& t) {
  vector<InstantiatedType*> typeParamTypes = getTypeParams();
  InstantiatedType* retType = getReturnType();

  // TODO: assert only two cases:
  //   1) full type instantiation
  //   2) no instantiations
  // In other words, no partial instantiations allowed

  SemanticContext* ctx = getSymbolTable()->getSemanticContext();

  BaseSymbol *bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);

  BoundFunction bf(static_cast<FuncSymbol*>(bs), typeParamTypes);
  BoundFunction translatedBF;
  t.translate(ctx, bf, translatedBF);

  if (translatedBF.isFullyInstantiated()) {
    InstantiatedType::AssertNoTypeParamPlaceholders(translatedBF.second);
    return new FuncDeclNodeParser(
      translatedBF.createFuncName(),
      StrVec(),
      util::transform_vec(params.begin(), params.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)),
      t.translate(ctx, retType)
        ->toParameterizedString(symbols),
      stmts->cloneForTemplate(t));
  } else {
    return new FuncDeclNodeParser(
      name,
      getTypeParamNames(),
      util::transform_vec(params.begin(), params.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)),
      t.translate(ctx, retType)
        ->toParameterizedString(symbols),
      stmts->cloneForTemplate(t));
  }
}

void
FuncDeclNodeParser::checkAndInitTypeParams(SemanticContext* ctx) {
  // type params
  assert(typeParamTypes.empty());
  for (size_t pos = 0; pos < typeParams.size(); pos++) {
    // add all the type params into the body's symtab
    Type *type = ctx->createTypeParam(typeParams[pos], pos);
    typeParamTypes.push_back(type->instantiate(ctx));
    stmts->getSymbolTable()->createClassSymbol(
        typeParams[pos],
        ctx->getRootSymbolTable()->newChildScopeNoNode(),
        type);
  }
}

void
FuncDeclNodeParser::checkAndInitReturnType(SemanticContext* ctx) {
  assert(!retType);
  retType = retTypeString ?
    ctx->instantiateOrThrow(stmts->getSymbolTable(), retTypeString) :
    InstantiatedType::VoidType;
}

FuncDeclNode*
FuncDeclNodeParser::cloneImpl(CloneMode::Type type) {
	switch (type) {
	case CloneMode::Structural:
		return new FuncDeclNodeParser(
			name,
			typeParams,
			util::transform_vec(params.begin(), params.end(),
				ASTExpressionNode::CloneFunctor(type)),
			retTypeString ? retTypeString->clone() : NULL,
			stmts->clone(type));
	case CloneMode::Semantic:
		assert(typeParams.size() == typeParamTypes.size());
		assert(retType);
		return new FuncDeclNodeSynthetic(
			name,
			typeParamTypes,
			util::transform_vec(params.begin(), params.end(),
				ASTExpressionNode::CloneFunctor(type)),
			retType,
			stmts->clone(type));
	default: VENOM_NOT_REACHED;
	}
}

ASTStatementNode*
FuncDeclNodeParser::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

FuncDeclNode*
FuncDeclNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  return cloneForTemplateImplHelper(t);
}

void
CtorDeclNode::registerSymbol(SemanticContext* ctx) {
  assert(hasLocationContext(ASTNode::TopLevelClassBody));
  VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, symbols->getOwner());

  // rewrite:
  //
  // def self(...) : super(a0, a1, ...) = stmts end
  //
  // into
  //
  // def self(...) = super.<ctor>(a0, a1, ...); stmts end
  //
  // if such a call does not exist

  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
  StmtListNode* stmtListNode = static_cast<StmtListNode*>(stmts);

  bool ctorCallFound = false;

  { // pattern match for ctor

    // TODO: gross and fragile...
    if (stmtListNode->getNumKids())
      if (StmtExprNode* s1 =
            dynamic_cast<StmtExprNode*>(stmtListNode->getNthKid(0)))
        if (FunctionCallNode* s2 =
              dynamic_cast<FunctionCallNode*>(s1->getExpression()))
          if (AttrAccessNode* s3 =
                dynamic_cast<AttrAccessNode*>(s2->getPrimary()))
            if (s3->getName() == "<ctor>")
              if (dynamic_cast<VariableSuperNode*>(s3->getPrimary()))
                ctorCallFound = true;
  } // end pattern match

  if (!ctorCallFound) {
    // clone mode must be structural, because there are no
    // symbols at this point
    StmtExprNode *stmt =
      new StmtExprNode(
        new FunctionCallNodeParser(
            new AttrAccessNode(new VariableSuperNode, "<ctor>"),
            TypeStringVec(),
            util::transform_vec(superArgs.begin(), superArgs.end(),
              ASTExpressionNode::CloneFunctor(CloneMode::Structural))));

    stmtListNode->prependStatement(stmt);

    // stmt gets semanticCheck called *AFTER* this invocation
    // to registerSymbol(), so we don't need to call it manually
    stmt->initSymbolTable(stmts->getSymbolTable());
  }

  FuncDeclNode::registerSymbol(ctx);
}

CtorDeclNode*
CtorDeclNode::cloneImpl(CloneMode::Type type) {
  return new CtorDeclNode(
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneFunctor(type)),
    stmts->clone(type),
    util::transform_vec(superArgs.begin(), superArgs.end(),
      ASTExpressionNode::CloneFunctor(type)));
}

ASTStatementNode*
CtorDeclNode::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

CtorDeclNode*
CtorDeclNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new CtorDeclNode(
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneTemplateFunctor(t)),
    stmts->cloneForTemplate(t),
    util::transform_vec(superArgs.begin(), superArgs.end(),
      ASTExpressionNode::CloneTemplateFunctor(t)));
}

FuncDeclNode*
CtorDeclNode::newFuncDeclNodeForLift(
      LiftContext& ctx,
      const string& name,
      const ExprNodeVec& params,
      InstantiatedType* returnType,
      ASTStatementNode* stmts) {
  assert(name == "<ctor>");
  assert(returnType->isVoid());
  return new CtorDeclNode(
      params,
      stmts,
      util::transform_vec(superArgs.begin(), superArgs.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

}
}
