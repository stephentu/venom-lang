#include <algorithm>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/expression/variable.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

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
  if (symbols->isDefined(name, SymbolTable::Any, false)) {
    throw SemanticViolationException(
        "Function/Method " + name + " already defined");
  }

  // check duplicate param names
  vector<string> names;
  names.resize(params.size());
  transform(params.begin(), params.end(), names.begin(), name_functor);
  if (!is_unique(names.begin(), names.end())) {
    throw SemanticViolationException("Duplicate parameter names");
  }

  // check and instantiate parameter types
  vector<InstantiatedType*> itypes;
  itypes.resize(params.size());
  transform(params.begin(), params.end(),
            itypes.begin(), itype_functor(ctx, symbols));

  // check and instantiate return type
  InstantiatedType* retType;
  if (ret_typename) {
    retType = ctx->instantiateOrThrow(symbols, ret_typename);
  } else {
    // treat no ret type as void type
    retType = InstantiatedType::VoidType;
  }


  if (locCtx & ASTNode::TopLevelClassBody) {
    ClassDeclNode *cdn = dynamic_cast<ClassDeclNode*>(symbols->getOwner());
    assert(cdn);

    // check that ctors dont have non-void return types
    if (cdn->getName() == name &&
        !retType->equals(*InstantiatedType::VoidType)) {
      throw SemanticViolationException(
          "Constructor cannot have non void return type");
    }

    // check that type-signature matches for overrides
    FuncSymbol *fs = symbols->findFuncSymbol(name, true);
    if (fs && fs->isMethod()) {
      InstantiatedType *overrideType =
        fs->bind(ctx, vector<InstantiatedType*>());

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
  symbols->createFuncSymbol(name, itypes, retType);

  // add parameters to block (child) symtab
  for (size_t i = 0; i < params.size(); i++) {
    VariableNode *vn = dynamic_cast<VariableNode*>(params[i]);
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), itypes[i]);
  }
}

}
}
