#include <algorithm>
#include <utility>

#include <ast/expression/functioncall.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;
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
FunctionCallNode::typeCheckImpl(SemanticContext*  ctx,
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
  } else if (!funcType->getType()->isFunction()) {
    throw TypeViolationException(
        "Type " + funcType->stringify() + " is not invocable");
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

void
FunctionCallNode::codeGen(CodeGenerator& cg) {
  InstantiatedType *funcType = primary->getStaticType();
  if (funcType->getType()->isClassType()) {
    VENOM_UNIMPLEMENTED;
  } else {
    // push arguments onto stack in reverse order
    for (ExprNodeVec::reverse_iterator it = args.rbegin();
         it != args.rend(); ++it) {
      (*it)->codeGen(cg);
    }
    // emit call
    BaseSymbol *bs = primary->getSymbol();
    if (!bs) {
      // TODO: implement calling of function objects
      VENOM_UNIMPLEMENTED;
    }
    FuncSymbol *fs = dynamic_cast<FuncSymbol*>(bs);
    assert(fs);
    size_t fidx = cg.enterFunction(fs);
    if (fs->isMethod()) {
      // emit the "this" pointer
      primary->codeGen(cg);
      // TODO: lookup the method index in the vtable
      VENOM_UNIMPLEMENTED;
    } else {
      cg.emitInstU32(
          !fs->isNative() ? Instruction::CALL : Instruction::CALL_NATIVE,
          fidx);
    }
  }
}

FunctionCallNode*
FunctionCallNode::cloneImpl() {
  return new FunctionCallNode(
      primary->clone(),
      util::transform_vec(
        typeArgs.begin(), typeArgs.end(),
        ParameterizedTypeString::CloneFunctor()),
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneFunctor()));
}

}
}
