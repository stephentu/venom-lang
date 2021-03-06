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
#include <analysis/type.h>

#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>
#include <ast/expression/synthetic/variable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>
#include <ast/statement/stmtexpr.h>
#include <ast/statement/stmtlist.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
StmtListNode::registerSymbol(SemanticContext* ctx) {
  if (getSymbolTable()->isModuleLevelSymbolTable()) {
    // create module type
    Type *moduleType = ctx->createModuleType(ctx->getModuleName());

    // create the module class symbol (in the *ROOT* symbol table
    // (which is the parent of this node's symbol table))
    ClassSymbol* moduleClassSymbol =
      ctx->getRootSymbolTable()->createClassSymbol(
        moduleType->getName(), getSymbolTable(), moduleType);

    // create the symbol for the module object singleton
    ctx->getRootSymbolTable()->createModuleSymbol(
        ctx->getModuleName(), getSymbolTable(), moduleClassSymbol, ctx);
  }
}

void
StmtListNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  if (stmts.empty()) {
    checkExpectedType(expected);
  } else {
    for (size_t i = 0; i < stmts.size(); i++) {
      if (i == stmts.size() - 1) {
        stmts[i]->typeCheck(ctx, expected);
      } else {
        stmts[i]->typeCheck(ctx);
      }
    }
  }
}

struct _type_less_cmp {
  inline bool operator()(const Type* lhs,
                         const Type* rhs) const {
    assert(lhs->getClassSymbol());
    assert(rhs->getClassSymbol());
    return lhs->getClassSymbol() < rhs->getClassSymbol();
  }
};

typedef map<Type*, vector<InstantiatedType*>, _type_less_cmp> TypeMap;

// TODO: reconcile insertSpecializedTypes() and insertSpecializedFunctions() -
// they are both very similar

void
StmtListNode::insertSpecializedTypes(
    InstantiatedType* type,
    const vector<ClassDeclNode*>& classDecls) {

  ASTNode *node = type->getClassSymbolTable()->getOwner();
  VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, node);
  ClassDeclNode *classNode = static_cast<ClassDeclNode*>(node);
  StmtNodeVec::iterator pos =
    find(stmts.begin(), stmts.end(), classNode);
  assert(pos != stmts.end());

#ifndef NDEBUG
  // assert the ClassDeclNodes actually belong here
  for (vector<ClassDeclNode*>::const_iterator it = classDecls.begin();
       it != classDecls.end(); ++it) {
    assert((*it)->getInstantiationOfType());
    assert((*it)->getInstantiationOfType()
                ->getType()->equals(*type->getType()));
    assert((*it)->getSymbolTable() == symbols);
  }
#endif /* NDEBUG */

  stmts.insert(pos + 1, classDecls.begin(), classDecls.end()); // insert after
  for (vector<ClassDeclNode*>::const_iterator it = classDecls.begin();
       it != classDecls.end(); ++it) {
    (*it)->setLocationContext(locCtx);
  }
}

void
StmtListNode::insertSpecializedFunctions(
    FuncSymbol* function,
    const vector<FuncDeclNode*>& funcDecls) {

  ASTNode *node = function->getFunctionSymbolTable()->getOwner();
  VENOM_ASSERT_TYPEOF_PTR(FuncDeclNode, node);
  FuncDeclNode *funcNode = static_cast<FuncDeclNode*>(node);
  StmtNodeVec::iterator pos =
    find(stmts.begin(), stmts.end(), funcNode);
  assert(pos != stmts.end());

#ifndef NDEBUG
  // assert the FuncDeclNodes actually belong here
  for (vector<FuncDeclNode*>::const_iterator it = funcDecls.begin();
       it != funcDecls.end(); ++it) {
    assert((*it)->getSymbolTable() == symbols);
  }
#endif /* NDEBUG */

  stmts.insert(pos + 1, funcDecls.begin(), funcDecls.end()); // insert after
  for (vector<FuncDeclNode*>::const_iterator it = funcDecls.begin();
       it != funcDecls.end(); ++it) {
    (*it)->setLocationContext(locCtx);
  }
}

ASTNode*
StmtListNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  if (mode != ModuleMain) return ASTNode::rewriteLocal(ctx, mode);

  if (getSymbolTable()->isModuleLevelSymbolTable()) {
    // we move all the non function/class definitions into
    // a separate function we call <main>

    StmtNodeVec mainStmts;
    for (StmtNodeVec::iterator it = stmts.begin();
         it != stmts.end();) {
      ASTStatementNode* kid = *it;
      if (dynamic_cast<FuncDeclNode*>(kid) ||
          dynamic_cast<ClassDeclNode*>(kid)) {
        ++it; continue;
      }
      it = stmts.erase(it);
      mainStmts.push_back(kid);
    }

    // create a function with mainStmts
    StmtListNode* mainStmtsNode = new StmtListNode(mainStmts);
    FuncDeclNode* mainFcn = new FuncDeclNodeParser(
        "<main>", util::StrVec(),
        ExprNodeVec(), NULL, mainStmtsNode);
    // <main> is a strange function when it comes to its symbol table
    mainFcn->setSymbolTable(symbols);
    //mainStmtsNode->setSymbolTable(symbols);
    mainFcn->registerSymbol(ctx);

    // don't typecheck the new function (no types changed)

    // append to the stmt list
    stmts.push_back(mainFcn);
  }
  return NULL;
}

void
StmtListNode::liftRecurseAndInsert(SemanticContext* ctx) {
  assert(ctx == symbols->getSemanticContext());
  for (size_t i = 0; i < stmts.size(); i++) {
    if (stmts[i]->isTypeParameterized()) continue;
    vector<ASTStatementNode*> liftedStmts;
    stmts[i]->liftPhaseImpl(ctx, symbols, liftedStmts, false);

    bool isClass = dynamic_cast<ClassDeclNode*>(stmts[i]);

#ifndef NDEBUG
    if (isClass) {
      // should only be lifting classes out of classes
      for (vector<ASTStatementNode*>::iterator it = liftedStmts.begin();
           it != liftedStmts.end(); ++it) {
        VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, *it);
      }
    }
#endif /* NDEBUG */

    // insert liftedStmts before stmt
    stmts.insert(stmts.begin() + i,
                 liftedStmts.begin(), liftedStmts.end());
    i += liftedStmts.size();
  }
}

void
StmtListNode::liftPhase(SemanticContext* ctx) {
  assert(getSymbolTable()->isModuleLevelSymbolTable());
  assert(!isTypeParameterized());
  liftRecurseAndInsert(ctx);
}

static inline void LiftPhaseImplStmtAssertions(ASTStatementNode* stmt) {
#ifndef NDEBUG
  if (FuncDeclNode* func = dynamic_cast<FuncDeclNode*>(stmt)) {
    // should never lift a ctor
    assert(!func->isCtor());
  }
#endif /* NDEBUG */
}

// TODO: make getName() a virtual function on ASTNode,
// so we don't have to keep doing this...
static inline string ExtractName(ASTStatementNode* stmt) {
  if (FuncDeclNode* func = dynamic_cast<FuncDeclNode*>(stmt)) {
    return func->getName();
  } else if (ClassDeclNode* klass = dynamic_cast<ClassDeclNode*>(stmt)) {
    return klass->getName();
  }
  VENOM_NOT_REACHED;
}

void
StmtListNode::liftPhaseImpl(SemanticContext* ctx,
                            SymbolTable* liftInto,
                            vector<ASTStatementNode*>& liftedStmts,
                            bool excludeFunctions) {
  liftRecurseAndInsert(ctx);
  LiftContext::LiftMap liftMap;
  set<BaseSymbol*> refs;
  for (vector<ASTStatementNode*>::iterator it = stmts.begin();
       it != stmts.end(); ) {
    ASTStatementNode* stmt = *it;
    bool isFunc = dynamic_cast<FuncDeclNode*>(stmt);
    bool isClass = dynamic_cast<ClassDeclNode*>(stmt);
    if ((isFunc && !excludeFunctions) || isClass) {

      // sanity checks
      LiftPhaseImplStmtAssertions(stmt);

      // don't lift type param functions/classes (since this runs after type
      // specialization)
      if (stmt->isTypeParameterized()) {
        ++it;
        continue;
      }

      // create a name for the lifted function
      string liftedName =
        ExtractName(stmt) + "$lifted_" + util::stringify(ctx->uniqueId());

      LiftContext liftCtx
        (stmt->getSymbol(), liftedName, symbols, liftInto, liftMap);

      // gather all non-local refs first
      stmt->collectNonLocalRefs(liftCtx);

      // insert to refs
      refs.insert(liftCtx.refs.vec.begin(), liftCtx.refs.vec.end());

      // clone/rewrite the function/class
      ASTStatementNode* clone =
        CloneForLift<ASTStatementNode, ASTStatementNode>(stmt, liftCtx);

      // need to set context correctly
      clone->setLocationContext(liftInto->getNode()->getLocationContext());

      // add to liftMap
      assert(liftMap.find(stmt->getSymbol()) == liftMap.end());
      liftMap[stmt->getSymbol()] = make_pair(liftCtx.refs.vec, clone);

      // remove the orig func stmt from the stmt list

      // TODO: We have a memory leak here!
      //
      // However the problem is, class symbols still store a pointer
      // to the deleted node. so we cannot really free it here.
      //
      // Eventually, we will use some sort of ref-counting pointer type,
      // so that we don't have to worry about these memory issues.
      //
      // For now, we document the memory leak
      it = stmts.erase(it);

      // register the lifted symbol into the scope it is being
      // lifted into. We need to this this *here*, so that when
      // we call rewriteAfterLift(), the symbols will be valid
      clone->initSymbolTable(liftInto);
      clone->semanticCheck(ctx);

      // mark lifted class
      // WARNING: this *MUST* happen before typeChecking clone
      if (isClass) {
        VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, stmt->getSymbol());
        VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, clone->getSymbol());
        static_cast<ClassSymbol*>(stmt->getSymbol())
          ->setLifted(static_cast<ClassSymbol*>(clone->getSymbol()));
      }

      clone->typeCheck(ctx);

      liftedStmts.push_back(clone);
    } else {
      ++it;
    }
  }

  // since this phase runs *after* template instantiation,
  // need to instantiate ref types manually. while this may seem
  // sub-optimal (why don't we just run lifting before template
  // instantiation), this is probably the easier approach, since
  // having to worry about type parameters while lifting is a bit
  // more tricky (not impossible, but would be more code than this...)
  SymbolTable* rootTable =
    ctx->getProgramRoot()->getRootSymbolTable();
  for (set<BaseSymbol*>::iterator it = refs.begin();
       it != refs.end(); ++it) {
    VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
    Symbol* sym = static_cast<Symbol*>(*it);

    // TODO: this is somewhat copy/pasted from
    // parser/driver.cc. Should refactor this out
    InstantiatedType* itype = sym->getInstantiatedType()->refify(ctx);
    TypeTranslator t;
    ClassSymbol* csym =
      rootTable->findClassSymbol(
          itype->createClassName(),
          SymbolTable::NoRecurse, t);
    if (!csym) {
      TypeTranslator t;
      t.bind(itype);
      itype->getClassSymbol()->instantiateSpecializedType(t);
    }
  }

  StmtNodeVec postPrepend;
  if (hasLocationContext(TopLevelFuncBody)) {
    // need to possibly rewrite parameters, for a case like this:
    // def f(a::int) =
    //   def g() = ..assign to a.. end
    //   ...
    // end
    FuncDeclNode* enclosing = getEnclosingFuncNode();
    assert(enclosing);

    // scan param list (it *won't* include lifted refs),
    // looking for refs we need to instantiate (and mark)
    ExprNodeVec& params = enclosing->getParams();
    for (ExprNodeVec::iterator it = params.begin();
         it != params.end(); ++it) {
      VENOM_ASSERT_TYPEOF_PTR(VariableNode, *it);
      VariableNode* vn = static_cast<VariableNode*>(*it);
      TypeTranslator t;
      Symbol* s =
        symbols->findSymbol(vn->getName(), SymbolTable::NoRecurse, t);
      assert(s);
      set<BaseSymbol*>::iterator sit = refs.find(s);
      if (sit == refs.end()) continue;

      // create new symbol
      Symbol* repSym = symbols->createSymbol(vn->getName() + "$renamed",
                                             s->getInstantiatedType(),
                                             enclosing);

      // replace old param
      *it = new VariableNodeSynthetic(
          repSym->getName(), s->getInstantiatedType());

      // delete old param
      delete vn;

      // insert instantiation stmts
      ExprListNode* exprs = new ExprListNode;
      StmtExprNode* stmtexpr = new StmtExprNode(exprs);

      // mark sym
      s->markPromoteToRef();

      exprs
        // param = ref{type}();
        ->appendExpression(
          new AssignExprNode(
            new VariableNodeParser(s->getName(), NULL),
            new FunctionCallNodeSynthetic(
              new VariableNodeParser(
                s->getInstantiatedType()->refify(ctx)->createClassName(),
                NULL),
              InstantiatedTypeVec(),
              ExprNodeVec())))
        // param.value = param$renamed;
        ->appendExpression(
          new AssignExprNode(
            new AttrAccessNode(
              new VariableNodeParser(s->getName(), NULL), "value"),
            new VariableNodeParser(repSym->getName(), NULL)));

      // get ready to insert this stmtexpr into the stmt body
      stmtexpr->initSymbolTable(symbols);
      stmtexpr->semanticCheck(ctx);
      stmtexpr->typeCheck(ctx);

      // but don't actually do it yet (buffer it up instead)
      postPrepend.push_back(stmtexpr);
    }
  }

  // now, rewrite the references in *this* scope to reflect the
  // *lifted* symbols
  ASTNode* retVal = rewriteAfterLift(liftMap, refs);
  VENOM_ASSERT_NULL(retVal);

  // now, prepend. we do this *after* rewriteAfterLift(), so
  // we don't rewrite the statements we insert (since they
  // are not meant to be rewritten
  for (StmtNodeVec::reverse_iterator it = postPrepend.rbegin();
       it != postPrepend.rend(); ++it) {
    prependStatement(*it);
  }
}

ASTNode*
StmtListNode::rewriteReturn(SemanticContext* ctx) {
  if (stmts.empty()) {
    ReturnNode* retNode = new ReturnNode(NULL);
    appendStatement(retNode);
    retNode->initSymbolTable(symbols);
  } else {
    ASTNode *last = stmts.back();
    ASTNode *rep = last->rewriteReturn(ctx);
    if (rep) {
      assert(rep != last);
      setNthKid(stmts.size() - 1, rep);
      delete last;
    }
  }
  return NULL;
}

void
StmtListNode::codeGen(CodeGenerator& cg) {
  if (getSymbolTable() && getSymbolTable()->isModuleLevelSymbolTable()) {
    // find the module symbol
    SemanticContext* ctx = getSymbolTable()->getSemanticContext();
    ModuleSymbol* msym = ctx->getRootSymbolTable()->findModuleSymbol(
        ctx->getModuleName(), SymbolTable::NoRecurse);
    VENOM_ASSERT_NOT_NULL(msym);

    // need to register the module class
    bool create;
    cg.enterLocalClass(
        msym->getModuleClassSymbol()->getType()->instantiate(ctx), create);
    assert(create);
  }
  ASTNode::codeGen(cg);
}

StmtListNode*
StmtListNode::cloneImpl(CloneMode::Type type) {
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
        ASTStatementNode::CloneFunctor(type)));
}

ASTStatementNode*
StmtListNode::cloneForLiftImpl(LiftContext& ctx) {
  StmtNodeVec filteredStmts;
  filteredStmts.reserve(stmts.size());
  for (StmtNodeVec::iterator it = stmts.begin();
       it != stmts.end(); ++it) {
    if (!(*it)->isTypeParameterized()) filteredStmts.push_back(*it);
  }
  return new StmtListNode(
      util::transform_vec(filteredStmts.begin(), filteredStmts.end(),
        ASTStatementNode::CloneLiftFunctor(ctx)));
}

StmtListNode*
StmtListNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
        ASTStatementNode::CloneTemplateFunctor(t)));
}

}
}
