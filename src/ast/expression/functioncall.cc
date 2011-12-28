#include <algorithm>

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
  inline InstantiatedType* operator()(ASTExpressionNode* node) const {
    return node->typeCheck(ctx, NULL);
  }
  SemanticContext* ctx;
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
                            InstantiatedType* expected) {
  InstantiatedType *funcType = primary->typeCheck(ctx, NULL);
  InstantiatedType *origType = funcType;

  bool isCtor = false;
  if (!funcType->isFunction()) {
    // try to find the constructor
    TypeTranslator t;
    BaseSymbol *ctorSym = funcType
      ->getClassSymbolTable()
      ->findBaseSymbol(funcType->getType()->getName(),
                       SymbolTable::Function,
                       false, t);
    assert(ctorSym);
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

  vector<InstantiatedType*> paramTypes(args.size());
  transform(args.begin(), args.end(), paramTypes.begin(), functor(ctx));

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

  return isCtor ? origType : funcType->getParams().back();

}

}
}
