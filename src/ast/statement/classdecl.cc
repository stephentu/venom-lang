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
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/expression/synthetic/variable.h>

#include <ast/statement/classattrdecl.h>
#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtlist.h>

#include <ast/statement/synthetic/classdecl.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

StmtListNode*
ClassDeclNode::getStmts() {
  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
  return static_cast<StmtListNode*>(stmts);
}

InstantiatedType*
ClassDeclNode::getSelfType(SemanticContext* ctx) {
  TypeTranslator t;
  ClassSymbol *cs = getSymbolTable()->findClassSymbol(
        getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getSelfType(ctx);
}

void
ClassDeclNode::registerSymbol(SemanticContext* ctx) {
  // check to see if this class is already defined in this scope
  if (symbols->isDefined(name, SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Class " + name + " already defined");
  }

  // type params
  checkAndInitTypeParams(ctx);

  // parents
  checkAndInitParents(ctx);
  vector<InstantiatedType*> parents = getParents();

  assert(parents.size() >= 1);
  if (parents.size() > 1) {
    throw SemanticViolationException(
        "Multiple inheritance currently not supported");
  }

  registerClassSymbol(ctx, parents, getTypeParams());
}

void
ClassDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }

  // register all the children first
  forchild (kid) {
    kid->registerSymbol(ctx);
  } endfor

  // now recurse on children w/o registration
  forchild (kid) {
    kid->semanticCheckImpl(ctx, false);
  } endfor

  // now look for a ctor definition
  TypeTranslator t;
  if (!stmts->getSymbolTable()->findFuncSymbol(
        "<ctor>", SymbolTable::NoRecurse, t)) {
    // no ctor defined, insert a default one
    CtorDeclNode *ctor =
      new CtorDeclNode(ExprNodeVec(), new StmtListNode, ExprNodeVec());
    VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
    static_cast<StmtListNode*>(stmts)->appendStatement(ctor);

    ctor->initSymbolTable(stmts->getSymbolTable());
    ctor->registerSymbol(ctx);
    ctor->semanticCheckImpl(ctx, false); // technically not needed,
                                         // since empty body
  }
  assert(stmts->getLocationContext() & TopLevelClassBody);
  assert(stmts->getSymbolTable()->findFuncSymbol(
          "<ctor>", SymbolTable::NoRecurse, t));
}

void
ClassDeclNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  vector<InstantiatedType*> parents = getParents();
  for (vector<InstantiatedType*>::iterator it = parents.begin();
       it != parents.end(); ++it) {
    InstantiatedType* itype = t.translate(ctx, *it);
    if (itype->isSpecializedType()) callback.offerType(itype);
  }
  // don't recurse unless this class decl is instantiated
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, getSymbol());
  ClassSymbol* csym = static_cast<ClassSymbol*>(getSymbol());
  if (t.translate(ctx, csym->getSelfType(ctx))->isFullyInstantiated()) {
    ASTNode::collectSpecialized(ctx, t, callback);
  }
}

void
ClassDeclNode::liftPhaseImpl(SemanticContext* ctx,
                             SymbolTable* liftInto,
                             vector<ASTStatementNode*>& liftedStmts,
                             bool excludeFunctions) {
  stmts->liftPhaseImpl(ctx, liftInto, liftedStmts, true);
}

// TODO: don't really need this override
void
ClassDeclNode::codeGen(CodeGenerator& cg) {
  assert(!isTypeParameterized());
  ASTStatementNode::codeGen(cg);
}

BaseSymbol*
ClassDeclNode::getSymbol() {
  TypeTranslator t;
  return symbols->findClassSymbol(name, SymbolTable::NoRecurse, t);
}

ClassSymbol*
ClassDeclNode::getClassSymbol() {
  BaseSymbol* bsym = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, bsym);
  return static_cast<ClassSymbol*>(bsym);
}

void
ClassDeclNode::createClassSymbol(
    const string& name,
    SymbolTable* classTable,
    Type* type,
    const vector<InstantiatedType*>& typeParams) {
  if (instantiation) {
    assert(typeParams.empty());
    symbols->createSpecializedClassSymbol(
        classTable, instantiation, type);
  } else {
    symbols->createClassSymbol(name, classTable, type, typeParams);
  }
}

void
ClassDeclNode::registerClassSymbol(
    SemanticContext* ctx,
    const vector<InstantiatedType*>& parentTypes,
    const vector<InstantiatedType*>& typeParamTypes) {

  // Define this class's symbol. This MUST happen before semantic checks
  // on the children (to support self-references)
  // TODO: support multiple inheritance
  Type *type =
    ctx->createType(name, parentTypes.front(), typeParamTypes.size());
  createClassSymbol(name, stmts->getSymbolTable(),
                    type, typeParamTypes);
}

ASTStatementNode*
ClassDeclNode::cloneForLiftImplHelper(LiftContext& ctx) {
  assert(!isTypeParameterized());
  assert(ctx.isLiftingClass());
  assert(ctx.curLiftSym == getSymbol());

  ASTStatementNode* stmtsClone = stmts->cloneForLift(ctx);
  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);

  if (isNestedClass() && !getClassSymbol()->hasOuterReference()) {
    ClassDeclNode* outerCdn = getEnclosingClassNode();
    assert(outerCdn);
    ClassSymbol* csym = outerCdn->getClassSymbol();
    SemanticContext* sctx = ctx.definedIn->getSemanticContext();
    ClassAttrDeclNode *cattr =
      new ClassAttrDeclNode(
        new VariableNodeParser(
          "<outer>",
          csym->getSelfType(sctx)->toParameterizedString(ctx.liftInto)),
        NULL,
        true /* private class attr */);
    static_cast<StmtListNode*>(stmtsClone)->prependStatement(cattr);
  }

  // now add ref versions of lifted symbols into
  // class body definition
  for (vector<BaseSymbol*>::iterator it = ctx.refs.vec.begin();
       it != ctx.refs.vec.end(); ++it) {
    VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
    assert((*it)->getDefinedSymbolTable() == ctx.definedIn);
    Symbol* s = static_cast<Symbol*>(*it);
    InstantiatedType* refType =
      s->getInstantiatedType()->refify(ctx.definedIn->getSemanticContext());
    ClassAttrDeclNode *cattr =
      new ClassAttrDeclNode(
        new VariableNodeParser(
          ctx.refParamName(s), refType->toParameterizedString(ctx.liftInto)),
        NULL);
    static_cast<StmtListNode*>(stmtsClone)->prependStatement(cattr);
  }

  vector<InstantiatedType*> parents = getParents();

  vector<InstantiatedType*> tparents;
  tparents.reserve(parents.size());
  for (vector<InstantiatedType*>::iterator it = parents.begin();
       it != parents.end(); ++it) {
    tparents.push_back(
        (*it)->findCodeGeneratableIType(ctx.definedIn->getSemanticContext()));
  }

  vector<ParameterizedTypeString*> parentTypeStrs(tparents.size());
  transform(tparents.begin(), tparents.end(),
            parentTypeStrs.begin(),
            InstantiatedType::ToParameterizedStringFunctor(ctx.liftInto));

  return new ClassDeclNodeParser(
      ctx.liftedName,
      parentTypeStrs,
      util::StrVec(),
      stmtsClone,
      NULL);
}

ClassDeclNode*
ClassDeclNode::cloneForTemplateImplHelper(const TypeTranslator& t) {
  BaseSymbol* bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, bs);
  ClassSymbol* cs = static_cast<ClassSymbol*>(bs);
  SemanticContext* ctx = getSymbolTable()->getSemanticContext();
  InstantiatedType* itype =
    t.translate(ctx, cs->getSelfType(ctx));

  // TODO: assert only two cases:
  //   1) full type instantiation
  //   2) no instantiations
  // In other words, no partial instantiations allowed

  vector<InstantiatedType*> parentTypes = getParents();

  vector<InstantiatedType*> translated(parentTypes.size());
  transform(parentTypes.begin(), parentTypes.end(),
            translated.begin(), TypeTranslator::TranslateFunctor(ctx, t));

  vector<ParameterizedTypeString*> translatedTypeStrs(translated.size());
  transform(translated.begin(), translated.end(),
            translatedTypeStrs.begin(),
            InstantiatedType::ToParameterizedStringFunctor(symbols));

  if (itype->isSpecializedType()) {
    // instantiated type
    InstantiatedType::AssertNoTypeParamPlaceholders(itype);
    return new ClassDeclNodeParser(
        itype->createClassName(),
        translatedTypeStrs,
        util::StrVec(),
        stmts->cloneForTemplate(t),
        itype);
  } else {
    // regular
    return new ClassDeclNodeParser(
        name,
        translatedTypeStrs,
        getTypeParamNames(),
        stmts->cloneForTemplate(t),
        NULL);
  }
}

struct instantiate_functor {
  instantiate_functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline
  InstantiatedType* operator()(const ParameterizedTypeString* t) const {
    return ctx->instantiateOrThrow(st, t);
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

void
ClassDeclNodeParser::print(ostream& o, size_t indent) {
  o << "(class " << name << std::endl << util::indent(indent + 1);

  // stringify parents
  vector<string> sparents;
  sparents.resize(parents.size());
  transform(parents.begin(), parents.end(),
            sparents.begin(), ParameterizedTypeString::StringerFunctor());

  // parents
  o << "(parents (" <<
    util::join(sparents.begin(), sparents.end(), " ") <<
    "))" << std::endl << util::indent(indent + 1);

  // type params
  o << "(type-params (" <<
    util::join(typeParams.begin(), typeParams.end(), ",") <<
    "))" << std::endl << util::indent(indent + 1);

  // statements
  stmts->print(o, indent + 1);

  o << ")";
}

void
ClassDeclNodeParser::checkAndInitTypeParams(SemanticContext* ctx) {
  // type params
  assert(typeParamTypes.empty());
  typeParamTypes.reserve(typeParams.size());
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
ClassDeclNodeParser::checkAndInitParents(SemanticContext* ctx) {
  // check to see if all parents are defined
  // use stmts's symtab to capture the parameterized types
  assert(parentTypes.empty());
  parentTypes.resize(parents.size());
  transform(parents.begin(), parents.end(),
            parentTypes.begin(),
            instantiate_functor(ctx, stmts->getSymbolTable()));
  if (parents.empty()) {
    // if no explicit parents declared, parent is object
    parentTypes.push_back(InstantiatedType::ObjectType);
  }
}

ClassDeclNode*
ClassDeclNodeParser::cloneImpl(CloneMode::Type type) {
	switch (type) {
	case CloneMode::Structural:
		return new ClassDeclNodeParser(
				name,
				util::transform_vec(parents.begin(), parents.end(),
					ParameterizedTypeString::CloneFunctor()),
				typeParams, stmts->clone(type));
	case CloneMode::Semantic:
		assert(parents.empty() ?
						 parentTypes.size() == 1 :
						 parents.size() == parentTypes.size());
		assert(typeParamTypes.size() == typeParams.size());
		return new ClassDeclNodeSynthetic(
				name,
				parentTypes,
				typeParamTypes, stmts->clone(type));
	default: VENOM_NOT_REACHED;
	}
}

ASTStatementNode*
ClassDeclNodeParser::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

ClassDeclNode*
ClassDeclNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  return cloneForTemplateImplHelper(t);
}

}
}
