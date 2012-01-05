#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtexpr.h>
#include <ast/statement/stmtlist.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::util;

namespace venom {
namespace ast {

struct name_functor_t {
  inline string operator()(ASTExpressionNode* node) const {
    return dynamic_cast<VariableNode*>(node)->getName();
  }
} name_functor;

struct itype_functor {
  itype_functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline InstantiatedType* operator()(ASTExpressionNode* node) const {
    VariableNode *vn = dynamic_cast<VariableNode*>(node);
    assert(vn->getExplicitParameterizedTypeString());
    return ctx->instantiateOrThrow(
        st, vn->getExplicitParameterizedTypeString());
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

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
    assert(dynamic_cast<ClassDeclNode*>(symbols->getOwner()));

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
    VariableNode *vn = dynamic_cast<VariableNode*>(params[i]);
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), itypes[i]);
  }
}

void
FuncDeclNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  TypeTranslator t;
  FuncSymbol *fs = symbols->findFuncSymbol(name, SymbolTable::NoRecurse, t);
  assert(fs);
  stmts->typeCheck(ctx, fs->getReturnType());
  checkExpectedType(expected);
}

void
CtorDeclNode::registerSymbol(SemanticContext* ctx) {
  assert(locCtx & ASTNode::TopLevelClassBody);
  assert(dynamic_cast<ClassDeclNode*>(symbols->getOwner()));

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
  dynamic_cast<StmtListNode*>(stmts)->prependStatement(stmt);

  // stmt gets semanticCheck called *AFTER* this invocation
  // to registerSymbol(), so we don't need to call it manually
  stmt->initSymbolTable(stmts->getSymbolTable());

  FuncDeclNode::registerSymbol(ctx);
}

}
}
