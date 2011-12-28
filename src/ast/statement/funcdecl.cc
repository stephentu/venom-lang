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
  InstantiatedType* retType =
    ctx->instantiateOrThrow(stmts->getSymbolTable(), ret_typename);

  if (locCtx & ASTNode::TopLevelClassBody) {
    ClassDeclNode *cdn = dynamic_cast<ClassDeclNode*>(symbols->getOwner());
    assert(cdn);

    if (cdn->getName() == name) {
      // check that ctors dont have non-void return types
      if (!retType->equals(*InstantiatedType::VoidType)) {
        throw SemanticViolationException(
            "Constructor cannot have non void return type");
      }

      // check that ctors dont have type parameters
      if (!typeParamTypes.empty()) {
        throw SemanticViolationException(
            "Constructor cannot have type parameters");
      }
    }

    // check that type-signature matches for overrides
    TypeTranslator t;
    FuncSymbol *fs = symbols->findFuncSymbol(name, true, t);
    // TODO: this isn't entirely correct way to determine overriding- for
    // instance a nested class could define a method with the same name of a
    // method of an outer class, but its not an override in that case
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
  FuncSymbol *fs = symbols->findFuncSymbol(name, false, t);
  assert(fs);
  stmts->typeCheck(ctx, fs->getReturnType());
  checkExpectedType(expected);
}

}
}
