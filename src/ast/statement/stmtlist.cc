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
    return lhs->getName() < rhs->getName();
  }
};

typedef map<Type*, vector<InstantiatedType*>, _type_less_cmp> TypeMap;

void
StmtListNode::instantiateSpecializedTypes(
    const vector<InstantiatedType*>& types,
    vector<ClassDeclNode*>& classDecls) {
  assert(!types.empty());
  classDecls.reserve(types.size());

  // assert we are the module level stmt list
  assert(getSymbolTable()->isModuleLevelSymbolTable());

  // assert all the types belong to this module
#ifndef NDEBUG
  for (vector<InstantiatedType*>::const_iterator it = types.begin();
       it != types.end(); ++it) {
    SymbolTable* t = (*it)->getClassSymbol()->getDefinedSymbolTable();
    assert(t->getSemanticContext() == getSymbolTable()->getSemanticContext());
  }
#endif /* NDEBUG */

  // organize the InstantiatedTypes by their Types
  // WARNING: the double parens are *necessary* here
  TypeMap typeMap((_type_less_cmp()));
  for (vector<InstantiatedType*>::const_iterator it = types.begin();
       it != types.end(); ++it) {
    InstantiatedType* itype = *it;
    pair< TypeMap::iterator, bool > res =
      typeMap.insert(make_pair(itype->getType(), util::vec1(itype)));
    if (res.second) {
      // new elem, no-op
    } else {
      // old type, add to vec
      res.first->second.push_back(itype);
    }
  }

  // for each type, find the corresponding class decl, and make
  // the instantiation. insert the instantiations into the stmt list,
  // and append to classDecs
  for (TypeMap::iterator it = typeMap.begin();
       it != typeMap.end(); ++it) {
    Type *t = it->first;
    ASTNode *node = t->getClassSymbolTable()->getOwner();
    VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, node);
    ClassDeclNode *classNode = static_cast<ClassDeclNode*>(node);
    StmtNodeVec::iterator pos =
      find(stmts.begin(), stmts.end(), classNode);
    assert(pos != stmts.end());

    vector<InstantiatedType*>& instances = it->second;

    vector<ClassDeclNode*> toInsert;
    toInsert.reserve(instances.size());

    for (vector<InstantiatedType*>::iterator iit = instances.begin();
         iit != instances.end(); ++iit) {
      TypeTranslator t;
      t.bind(*iit);

      // instantiate
      ClassDeclNode* instantiation = CloneForTemplate(classNode, t);

      // process the new instantiation
      instantiation->initSymbolTable(getSymbolTable());
      instantiation->semanticCheck(getSymbolTable()->getSemanticContext());
      instantiation->typeCheck(getSymbolTable()->getSemanticContext());

      // mark to be inserted
      toInsert.push_back(instantiation);

      // return to user
      classDecls.push_back(instantiation);
    }

    // insert into stmt list
    stmts.insert(pos + 1, toInsert.begin(), toInsert.end());
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
  for (size_t i = 0; i < stmts.size(); i++) {
    vector<ASTStatementNode*> liftedStmts;
    assert(ctx == symbols->getSemanticContext());
    stmts[i]->liftPhaseImpl(ctx, symbols, liftedStmts);
    // insert liftedStmts before pos i
    stmts.insert(stmts.begin() + i,
                 liftedStmts.begin(), liftedStmts.end());
    i += liftedStmts.size();
  }
}

void
StmtListNode::liftPhase(SemanticContext* ctx) {
  assert(getSymbolTable()->isModuleLevelSymbolTable());
  liftRecurseAndInsert(ctx);
}

void
StmtListNode::liftPhaseImpl(SemanticContext* ctx,
                            SymbolTable* liftInto,
                            vector<ASTStatementNode*>& liftedStmts) {
  liftRecurseAndInsert(ctx);
  LiftContext::LiftMap liftMap;
  set<BaseSymbol*> refs;
  for (vector<ASTStatementNode*>::iterator it = stmts.begin();
       it != stmts.end(); ) {
    ASTStatementNode* stmt = *it;
    if (FuncDeclNode* func = dynamic_cast<FuncDeclNode*>(stmt)) {
      // should never lift a ctor
      assert(!func->isCtor());

      // create a name for the lifted function
      string liftedName =
        func->getName() + "$lifted_" + util::stringify(ctx->uniqueId());

      LiftContext liftCtx(func->getSymbol(), liftedName, symbols, liftMap);

      // gather all non-local refs first
      stmt->collectNonLocalRefs(liftCtx);

      // insert to refs
      refs.insert(liftCtx.refs.vec.begin(), liftCtx.refs.vec.end());

      // clone/rewrite the function
      ASTStatementNode* clone =
        CloneForLift<FuncDeclNode, ASTStatementNode>(func, liftCtx);
      VENOM_ASSERT_TYPEOF_PTR(FuncDeclNode, clone);

      // add to liftMap
      assert(liftMap.find(func->getSymbol()) == liftMap.end());
      liftMap[func->getSymbol()] = make_pair(liftCtx.refs.vec, clone);

      // remove the orig func stmt from the stmt list
      it = stmts.erase(it);

      // register the lifted symbol into the scope it is being
      // lifted into. We need to this this *here*, so that when
      // we call rewriteAfterLift(), the symbols will be valid
      clone->initSymbolTable(liftInto);
      clone->semanticCheck(ctx);
      clone->typeCheck(ctx);

      liftedStmts.push_back(clone);

    } else if (ClassDeclNode* klass = dynamic_cast<ClassDeclNode*>(stmt)) {
      // TODO: implement me
      VENOM_ASSERT_NOT_NULL(klass);
      ++it;
    } else {
      ++it;
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
            new SymbolNode(s),
            new FunctionCallNodeSynthetic(
              new SymbolNode(Type::RefType->getClassSymbol()),
              util::vec1(s->getInstantiatedType()),
              ExprNodeVec())))
        // param.value = param$renamed;
        ->appendExpression(
          new AssignExprNode(
            new AttrAccessNode(new SymbolNode(s), "value"),
            new SymbolNode(repSym)));

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
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
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
