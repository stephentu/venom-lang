#include <algorithm>
#include <utility>

#include <ast/expression/functioncall.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::util;

namespace venom {
namespace ast {

struct functor {
  functor(SemanticContext* ctx) : ctx(ctx) {}
  inline InstantiatedType* operator()(
      const pair<ASTExpressionNode*, InstantiatedType*>& p) const {
    return p.first->typeCheck(ctx, p.second);
  }
  SemanticContext* ctx;
};

struct type_param_functor {
  type_param_functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline
  InstantiatedType* operator()(const ParameterizedTypeString* t) const {
    return ctx->instantiateOrThrow(st, t);
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

struct param_functor_t {
  inline bool operator()(InstantiatedType* lhs,
                         InstantiatedType* rhs) const {
    // lhs is the function type, rhs is the passed in type.
    // require rhs to be a subtype of lhs
    return !rhs->isSubtypeOf(*lhs);
  }
} param_functor;

InstantiatedType*
FunctionCallNode::typeCheck(SemanticContext*  ctx,
                            InstantiatedType* expected,
                            const InstantiatedTypeVec& typeParamArgs) {
  assert(typeParamArgs.empty());

  // instantiate type parameters
  InstantiatedTypeVec tparams(typeArgs.size());
  transform(typeArgs.begin(), typeArgs.end(),
            tparams.begin(), type_param_functor(ctx, symbols));

  InstantiatedType *funcType = primary->typeCheck(ctx, NULL, tparams);
  InstantiatedType *classType;
  bool isCtor = false;
  if (funcType->getType()->isClassType()) {
    // check to see if this scope can instantiate the type this is only
    // possible if the type exists as a top level type

    classType = funcType->getParams().at(0);
    assert(classType);

    if (!classType->getType()->getClassSymbol()->isTopLevelClass() &&
        !symbols->canSee(classType->getType()->getClassSymbol())) {
      throw TypeViolationException(
          "Cannot instantiate instance of nested subclass outside "
          "the context of the class");
    }

    // try to find the constructor
    TypeTranslator t;
    BaseSymbol *ctorSym = classType
      ->getClassSymbolTable()
      ->findBaseSymbol("<ctor>", SymbolTable::Function,
                       SymbolTable::NoRecurse, t);
    if (!ctorSym) {
      throw TypeViolationException(
          "Type " + funcType->stringify() + " is unconstructable");
    }
    t.bind(classType);
    funcType = ctorSym->bind(ctx, t, InstantiatedTypeVec());
    assert(funcType->isFunction());
    isCtor = true;
  }
  assert(funcType->getType()->getParams() > 0); // return type is last param
  if ((funcType->getType()->getParams() - 1) != args.size()) {
    throw TypeViolationException(
        "Wrong number of parameters to function type " +
        funcType->stringify() + ". Got " + stringify(args.size()) +
        ", expected " + stringify(funcType->getType()->getParams() - 1));
  }

  vector< pair<ASTExpressionNode*, InstantiatedType*> > zipped(args.size());
  zip(args.begin(), args.end(),
      funcType->getParams().begin(), zipped.begin());

  vector<InstantiatedType*> paramTypes(zipped.size());
  transform(zipped.begin(), zipped.end(), paramTypes.begin(), functor(ctx));

  vector<InstantiatedType*>::iterator it =
    binpred_find_if(funcType->getParams().begin(),
                    funcType->getParams().end() - 1,
                    paramTypes.begin(),
                    param_functor);
  if (it != (funcType->getParams().end() - 1)) {
    throw TypeViolationException(
        "Expected type " + (*it)->stringify() + ", got type " +
        paramTypes[it - funcType->getParams().begin()]->stringify());
  }

  return isCtor ? classType : funcType->getParams().back();
}

}
}
