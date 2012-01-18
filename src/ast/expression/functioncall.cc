#include <algorithm>
#include <utility>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>

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

  InstantiatedType *funcType = primary->typeCheck(ctx, NULL, getTypeParams());
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
    // new object

    InstantiatedType* klassType = funcType->getParams().at(0);
    ClassSymbol* csym = klassType->findSpecializedClassSymbol();
    TypeTranslator t;
    FuncSymbol* ctorSym =
      csym->getClassSymbolTable()->findFuncSymbol(
          "<ctor>", SymbolTable::NoRecurse, t);
    VENOM_ASSERT_NOT_NULL(ctorSym);

    bool create;
    size_t classIdx = cg.enterClass(klassType, create);

    // allocate the object
    cg.emitInstU32(Instruction::ALLOC_OBJ, classIdx);

    // store the obj in a temp location
    Symbol* tempSym = cg.createTemporaryVariable();
    size_t tempIdx = cg.createLocalVariable(tempSym, create);
    cg.emitInstU32(Instruction::STORE_LOCAL_VAR_REF, tempIdx);

    // push arguments to ctor onto stack in reverse order
    for (ExprNodeVec::reverse_iterator it = args.rbegin();
         it != args.rend(); ++it) {
      (*it)->codeGen(cg);
    }

    // push the "this" pointer, from temp storage
    cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, tempIdx);

    // call the ctor (directly, not as a virtual method)
    size_t refIdx = cg.enterFunction(ctorSym, create);
    cg.emitInstU32(
        ctorSym->isNative() ?
          Instruction::CALL_NATIVE :
          Instruction::CALL,
        refIdx);

    // ctors return void, so we need to pop off the null cell
    cg.emitInst(Instruction::POP_CELL_REF);

    // move the constructed obj onto the top of stack
    cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, tempIdx);

    // TODO: we could store a null into the temp location, to invalidate the
    // reference to this obj. but since venom does not do support strict
    // ref-count semantics, it is not necessary for correctness

    // return the temp location, to be re-used
    cg.returnTemporaryVariable(tempSym);

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
    VENOM_ASSERT_TYPEOF_PTR(FuncSymbol, bs);
    FuncSymbol *fs = static_cast<FuncSymbol*>(bs);
    bool create;
    size_t fidx = cg.enterFunction(fs, create);
    if (fs->isMethod()) {
      // emit the "this" pointer
      primary->codeGen(cg);

      // lookup the method index in the vtable
      VENOM_ASSERT_TYPEOF_PTR(MethodSymbol, fs);
      MethodSymbol* ms = static_cast<MethodSymbol*>(fs);

      // pattern match to see if we are dealing with
      // super.meth( ). this case + ctors are the only case
      // where we don't use virtual dispatch for methods
      bool isSuperInvoke = false;
      if (AttrAccessNode* attrAccess =
          dynamic_cast<AttrAccessNode*>(primary)) {
        if (dynamic_cast<VariableSuperNode*>(attrAccess->getPrimary())) {
          isSuperInvoke = true;
        }
      }

      if (ms->isConstructor() || isSuperInvoke) {
        // ctors are not invoked virtually
        cg.emitInstU32(
            !ms->isNative() ? Instruction::CALL : Instruction::CALL_NATIVE,
            fidx);
      } else {
        size_t slotIdx = ms->getFieldIndex();
        cg.emitInstU32(Instruction::CALL_VIRTUAL, slotIdx);
      }
    } else {
      cg.emitInstU32(
          !fs->isNative() ? Instruction::CALL : Instruction::CALL_NATIVE,
          fidx);
    }
  }
}

FunctionCallNode*
FunctionCallNodeParser::cloneImpl() {
  return new FunctionCallNodeParser(
      primary->clone(),
      util::transform_vec(
        typeArgs.begin(), typeArgs.end(),
        ParameterizedTypeString::CloneFunctor()),
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneFunctor()));
}

FunctionCallNode*
FunctionCallNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  assert(typeArgs.size() == typeArgTypes.size());
  return new FunctionCallNodeSynthetic(
      primary->cloneForTemplate(t),
      util::transform_vec(
        typeArgTypes.begin(), typeArgTypes.end(),
        TypeTranslator::TranslateFunctor(
          getSymbolTable()->getSemanticContext(), t)),
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)));
}

void
FunctionCallNodeParser::checkAndInitTypeParams(SemanticContext* ctx) {
  assert(typeArgTypes.empty());
  // instantiate type parameters
  typeArgTypes.resize(typeArgs.size());
  transform(typeArgs.begin(), typeArgs.end(),
            typeArgTypes.begin(),
            SemanticContext::InstantiateFunctor(ctx, symbols));
}

}
}
