#include <algorithm>
#include <set>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/return.h>
#include <ast/statement/stmtexpr.h>
#include <ast/statement/stmtlist.h>

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
  itype_functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline InstantiatedType* operator()(ASTExpressionNode* node) const {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, node);
    VariableNode *vn = static_cast<VariableNode*>(node);
    assert(vn->getExplicitParameterizedTypeString());
    return ctx->instantiateOrThrow(
        st, vn->getExplicitParameterizedTypeString());
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

FuncDeclNode::FuncDeclNode(const std::string&       name,
                           const util::StrVec&      typeParams,
                           const ExprNodeVec&       params,
                           ParameterizedTypeString* ret_typename,
                           ASTStatementNode*        stmts)
  : name(name), typeParams(typeParams), params(params),
    ret_typename(ret_typename), stmts(stmts) {

  stmts->addLocationContext(TopLevelFuncBody);
  for (ExprNodeVec::iterator it = this->params.begin();
       it != this->params.end(); ++it) {
    (*it)->addLocationContext(FunctionParam);
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
  assert(typeParamTypes.empty());
  InstantiatedTypeVec typeParamITypes;
  for (size_t pos = 0; pos < typeParams.size(); pos++) {
    // add all the type params into the body's symtab
    Type *type = ctx->createTypeParam(typeParams[pos], pos);
    typeParamTypes.push_back(type);
    typeParamITypes.push_back(type->instantiate(ctx));
    stmts->getSymbolTable()->createClassSymbol(
        typeParams[pos],
        ctx->getRootSymbolTable()->newChildScope(NULL),
        type);
  }

  // check and instantiate parameter types
  vector<InstantiatedType*> itypes(params.size());
  transform(params.begin(), params.end(),
            itypes.begin(), itype_functor(ctx, stmts->getSymbolTable()));

  // check and instantiate return type
  InstantiatedType* retType = ret_typename ?
    ctx->instantiateOrThrow(stmts->getSymbolTable(), ret_typename) :
    InstantiatedType::VoidType;

  if (!isCtor() && (locCtx & ASTNode::TopLevelClassBody)) {
    VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, symbols->getOwner());

    // check that type-signature matches for overrides
    TypeTranslator t;
    FuncSymbol *fs =
      symbols->findFuncSymbol(name, SymbolTable::ClassParents, t);
    if (fs && fs->isMethod()) {
      InstantiatedType *overrideType = fs->bind(ctx, t, typeParamITypes);

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
  }

  // add symbol to current symtab
  symbols->createFuncSymbol(name, typeParamITypes, itypes, retType);

  // add parameters to block (child) symtab
  for (size_t i = 0; i < params.size(); i++) {
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, params[i]);
    VariableNode *vn = static_cast<VariableNode*>(params[i]);
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), false, itypes[i]);
  }
}

BaseSymbol*
FuncDeclNode::getSymbol() {
  TypeTranslator t;
  FuncSymbol *fs = symbols->findFuncSymbol(name, SymbolTable::NoRecurse, t);
  return fs;
}

void
FuncDeclNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  BaseSymbol *bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);
  FuncSymbol *fs = static_cast<FuncSymbol*>(bs);

  stmts->typeCheck(ctx, fs->getReturnType());
  checkExpectedType(expected);
}

ASTNode*
FuncDeclNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
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

//void
//FuncDeclNode::lift(SemanticContext* ctx,
//                   vector<ASTStatementNode*>& liftedStmts,
//                   bool liftThisContext) {
//  // recurse first
//  vector<ASTStatementNode*> ls;
//  stmts->lift(ctx, ls, true);
//
//  // now lift all inner function decls
//  if (liftThisContext) {
//    assert(dynamic_cast<StmtListNode*>(stmts));
//    StmtListNode *funcStmts = static_cast<StmtListNode*>(stmts);
//    for (size_t i = 0; i < funcStmts->getNumKids(); i++) {
//      ASTNode *kid = funcStmts->getNthKid(i);
//      assert(kid);
//      if (FuncDeclNode *funcDecl = dynamic_cast<FuncDeclNode*>(kid)) {
//        // need to lift this decl- but first
//
//        // look for all locations in the func decl stmtlist which are non-local
//        // (reference a location defined in a parent). mark all the symbols
//        // ( which changes the type to ref{T} )
//        vector<Symbol*> nonLocalSyms;
//        funcDecl->getStmts()->findAndRewriteNonLocalRefs(ctx, nonLocalSyms);
//
//        // for each unique location, add a synthetic parameter to the func decl
//        set<Symbol*> uniqueNonLocalSyms(
//            nonLocalSyms.begin(), nonLocalSyms.end());
//
//        // rewrite all subsequent callers. the tricky case is:
//        //
//        //   def a() =
//        //     x = 1;
//        //     def b() = x = 2; end
//        //     def c() = b(); end
//        //     c();
//        //   end
//        //
//        // after b is lifted, we need to rewrite the caller of b in c.
//        // however, we'll need to pass the x reference into c as a param (even though
//        // c does not explicitly reference x). we can achieve this by rewriting the
//        // call in c to b as a$b(x) [where x is a reference to the original x symbol],
//        // and when we lift c the usual mechanism will take care of adding x as a
//        // param to the lifted c. thus, the final rewrite looks like:
//        //
//        //   def a$b(x::<ref>{int}) = x.set(2); end
//        //   def a$c(x::<ref>{int}) = a$b(x); end
//        //   def a() =
//        //     x = <ref>{int}(1);
//        //     a$c(x);
//        //   end
//
//
//      }
//    }
//
//
//  }
//}

void
FuncDeclNode::codeGen(CodeGenerator& cg) {
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
FuncDeclNode::cloneImpl() {
  return new FuncDeclNode(
    name,
    typeParams,
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneFunctor()),
    ret_typename ? ret_typename->clone() : NULL,
    stmts->clone());
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
  StmtExprNode *stmt =
    new StmtExprNode(
      new FunctionCallNode(
          new AttrAccessNode(new VariableSuperNode, "<ctor>"),
          TypeStringVec(),
          superArgs));

  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
  static_cast<StmtListNode*>(stmts)->prependStatement(stmt);

  // stmt gets semanticCheck called *AFTER* this invocation
  // to registerSymbol(), so we don't need to call it manually
  stmt->initSymbolTable(stmts->getSymbolTable());

  FuncDeclNode::registerSymbol(ctx);
}

CtorDeclNode*
CtorDeclNode::cloneImpl() {
  return new CtorDeclNode(
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneFunctor()),
    stmts->clone(),
    util::transform_vec(superArgs.begin(), superArgs.end(),
      ASTExpressionNode::CloneFunctor()));
}

}
}
