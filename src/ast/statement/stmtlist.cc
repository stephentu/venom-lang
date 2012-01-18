#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>
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
StmtListNode::cloneImpl() {
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
        ASTStatementNode::CloneFunctor()));
}

StmtListNode*
StmtListNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new StmtListNode(
      util::transform_vec(stmts.begin(), stmts.end(),
        ASTStatementNode::CloneTemplateFunctor(t)));
}

}
}
