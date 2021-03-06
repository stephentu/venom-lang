/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <utility>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/outer.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>

#include <ast/statement/synthetic/funcdecl.h>

#include <analysis/boundfunction.h>
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
  // lhs is the function type, rhs is the passed in type.
  // require rhs to be a subtype of lhs

  // TODO: this is a hack to work around a limitation in the
  // lifting system
  //
  // returns true if rhs is a lift of lhs
  inline bool checkLift(InstantiatedType* lhs,
                        InstantiatedType* rhs) const {
    return rhs->getClassSymbol()->isLiftOf(lhs->getClassSymbol()) ||
           rhs->isSpecializationOf(lhs);
  }
  inline bool operator()(InstantiatedType* lhs,
                         InstantiatedType* rhs) const {
    return !rhs->isSubtypeOf(*lhs) && !checkLift(lhs, rhs);
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
FunctionCallNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  ASTExpressionNode::collectSpecialized(ctx, t, callback);
  vector<InstantiatedType*> tparams = getTypeParams();
  if (tparams.empty()) return;
  if (!InstantiatedType::IsFullyInstantiated(tparams.begin(), tparams.end())) {
    return;
  }
  BaseSymbol* bs = primary->getSymbol();
  if (!bs) return;
  if (FuncSymbol* fs = dynamic_cast<FuncSymbol*>(bs)) {
    if (fs->isMethod()) {
      // harder case

      // Must find the source of this FuncSymbol.
      // It is not enough to grab the class symbol from FuncSymbol,
      // since that will not include the necessary type instantiations
      //
      // TODO: the fact that we have to do this reflects a poor
      // design in the semanticCheck/typeCheck phases. We should
      // fix this at some point
      //
      // Because rewriteLocal() has not run yet, there are two
      // cases we need to worry about:
      //
      // 1) expr.ident(...)
      // 2) ident(...)
      //
      // In case (1), we use the type of expr. In case (2), we use
      // the self type. One last caveat- in the case of a self-type,
      // we use the non-specialized type

      InstantiatedType* itype;
      if (AttrAccessNode* aan = dynamic_cast<AttrAccessNode*>(primary)) {
        // case 1
        itype = aan->getPrimary()->getStaticType();
      } else {
        // case 2
        VENOM_ASSERT_TYPEOF_PTR(VariableNode, primary);
        ClassDeclNode* cdn = getEnclosingClassNode();
        itype = cdn->getSelfType(ctx); // non-specialized type
      }
      InstantiatedType::AssertNoTypeParamPlaceholders(itype);

      InstantiatedType* klass;
      MethodSymbol* msToOffer =
        // find the original definition (w/ the original class)
        itype->findMethodSymbol(fs->getName(), klass, true);
      assert(msToOffer);
      assert(klass);
      InstantiatedType::AssertNoTypeParamPlaceholders(klass);

      BoundFunction bf(msToOffer, tparams);
      callback.offerMethod(klass, bf);
    } else {
      // easy case
      BoundFunction bf(fs, tparams);
      callback.offerFunction(bf);
    }
  }
}

BaseSymbol*
FunctionCallNode::extractSymbol(BaseSymbol* orig) {
  if (FuncSymbol* fs = dynamic_cast<FuncSymbol*>(orig)) {
    BoundFunction bf(fs, getTypeParams());
    BaseSymbol* ret = bf.findSpecializedFuncSymbol();
    return ret;
  } else if (ClassSymbol* klass = dynamic_cast<ClassSymbol*>(orig)) {
    SemanticContext* ctx = symbols->getSemanticContext();
    BaseSymbol* ret = klass
      ->getType()
      ->instantiate(ctx, getTypeParams())
      ->findSpecializedClassSymbol();
    return ret;
  }
  return NULL;
}

// TODO: make getName() a virtual function on ASTNode,
// so we don't have to keep doing this...
static inline string ExtractName(ASTStatementNode* stmt) {
  if (FuncDeclNode* func = dynamic_cast<FuncDeclNode*>(stmt)) {
    return func->getName();
  } else if (ClassDeclNode* klass = dynamic_cast<ClassDeclNode*>(stmt)) {
    return klass->getName();
  }
  VENOM_NOT_REACHED;
}

ASTNode*
FunctionCallNode::rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const set<BaseSymbol*>& refs) {
  // recurse first
  ASTNode* retVal = ASTExpressionNode::rewriteAfterLift(liftMap, refs);
  VENOM_ASSERT_NULL(retVal);

  BaseSymbol* bs = extractSymbol(primary->getSymbol());
  if (!bs) return NULL;
  LiftContext::LiftMap::const_iterator it = liftMap.find(bs);
  if (it == liftMap.end()) return NULL;

  const vector<BaseSymbol*>& liftedParams = it->second.first;
  string liftedName = ExtractName(it->second.second);

  // map each lifted param to a vector of names (possibly
  // adding the param to *this* scopes
  ExprNodeVec liftedParamExprs;
  liftedParamExprs.reserve(liftedParams.size() + args.size() + 1);

  if (ClassSymbol* csym = dynamic_cast<ClassSymbol*>(bs)) {
    VENOM_ASSERT_TYPEOF_PTR(
        ClassDeclNode, csym->getClassSymbolTable()->getOwner());
    ClassDeclNode* cdn =
      static_cast<ClassDeclNode*>(csym->getClassSymbolTable()->getOwner());
    if (cdn->isNestedClass()) {
      liftedParamExprs.push_back(new VariableSelfNode);
    }
  }

  for (vector<BaseSymbol*>::const_iterator it = liftedParams.begin();
       it != liftedParams.end(); ++it) {
    VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
    Symbol* sym = static_cast<Symbol*>(*it);
    assert(sym->isPromoteToRef());
    assert(!sym->isObjectField());
    set<BaseSymbol*>::const_iterator it = refs.find(sym);
    assert(it != refs.end());
    liftedParamExprs.push_back(new VariableNodeParser(sym->getName(), NULL));
  }

  // the regular params
  ExprNodeVec clonedExprs(args.size());
  transform(args.begin(), args.end(),
            clonedExprs.begin(),
            ASTExpressionNode::CloneFunctor(CloneMode::Structural));
  liftedParamExprs.insert(liftedParamExprs.end(),
                          clonedExprs.begin(), clonedExprs.end());

  // replace calling the rewritten function
  return replace(
      getSymbolTable()->getSemanticContext(),
      new FunctionCallNodeSynthetic(
        // don't need a SymbolNode here, b/c the name is guaranteed to be
        // unique (since it is a lifted name)
        new VariableNodeParser(liftedName, NULL),
        InstantiatedTypeVec(),
        liftedParamExprs));
}

void
FunctionCallNode::codeGen(CodeGenerator& cg) {
  InstantiatedType *funcType = primary->getStaticType();
  if (funcType->getType()->isClassType()) {
    // new object

    InstantiatedType* klassType = funcType->getParams().at(0);
    ClassSymbol* csym = klassType->findCodeGeneratableClassSymbol();
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

      if (isSuperInvoke) {
        // need to find the code-generatable symbol

        // get the super type
        InstantiatedType* superType =
          static_cast<AttrAccessNode*>(primary)->getPrimary()->getStaticType();

        // having to do this is sub-optimal, but it would be a lot of extra
        // work to propagate this information in the AST nodes...
        // TODO: eventually, we should fix this
        InstantiatedType* klass;
        MethodSymbol* msym = superType->findMethodSymbol(ms->getName(), klass);
        VENOM_ASSERT_NOT_NULL(msym);
        assert(klass);
        SymbolTable* lookup =
          klass->findCodeGeneratableClassSymbol()->getClassSymbolTable();
        TypeTranslator t;
        fs = lookup->findFuncSymbol(ms->getName(), SymbolTable::NoRecurse, t);
        VENOM_ASSERT_TYPEOF_PTR(MethodSymbol, fs);
        ms = static_cast<MethodSymbol*>(fs);

        // ctors are not invoked virtually
        bool create;
        size_t fidx = cg.enterFunction(ms, create);
        cg.emitInstU32(
            !ms->isNative() ? Instruction::CALL : Instruction::CALL_NATIVE,
            fidx);
      } else {
        assert(!ms->isConstructor());
        size_t slotIdx = ms->getFieldIndex();
        cg.emitInstU32(Instruction::CALL_VIRTUAL, slotIdx);
      }
    } else {
      vector<InstantiatedType*> typeParams = getTypeParams();
      BoundFunction bf(fs, typeParams);
      fs = bf.findSpecializedFuncSymbol();
      bool create;
      size_t fidx = cg.enterFunction(fs, create);
      cg.emitInstU32(
          !fs->isNative() ? Instruction::CALL : Instruction::CALL_NATIVE,
          fidx);
    }
  }
}

static ASTExpressionNode* createChainOfOuters(size_t n) {
  assert(n > 0);
  if (n == 1) return new VariableNodeParser("<outer>", NULL);
  return new OuterNode(createChainOfOuters(n - 1));
}

ASTExpressionNode*
FunctionCallNode::cloneForLiftImplHelper(LiftContext& ctx) {
  assert(getTypeParams().empty());

  BaseSymbol* psym = primary->getSymbol();
  if (psym) {

    // if we are invoking a constructor directly, then set the
    // symbol to the class symbol (since constructors should never be lifted
    // symbols by themselves)
    bool foundCtor = false;
    if (MethodSymbol* msym = dynamic_cast<MethodSymbol*>(psym)) {
      if (msym->isConstructor()) {
        assert(ctx.liftMap.find(msym) == ctx.liftMap.end());
        if (msym->getClassSymbol()->getUnliftedSymbol()) {
          psym = msym->getClassSymbol()->getUnliftedSymbol();
          foundCtor = true;
        }
      }
    }

    LiftContext::LiftMap::const_iterator it = ctx.liftMap.find(psym);
    if (it != ctx.liftMap.end()) {
      const vector<BaseSymbol*>& liftedParams = it->second.first;

      ExprNodeVec liftedParamExprs;
      liftedParamExprs.reserve(liftedParams.size() + args.size() + 1);

      // take care of <outer> reference first
      ClassSymbol* csym;
      if ((csym = dynamic_cast<ClassSymbol*>(psym)) &&
          (!csym->isLiftedClass() || foundCtor)) {
        VENOM_ASSERT_TYPEOF_PTR(
            ClassDeclNode, csym->getClassSymbolTable()->getOwner());
        ClassDeclNode* cdn =
          static_cast<ClassDeclNode*>(csym->getClassSymbolTable()->getOwner());
        if (cdn->isNestedClass()) {
          ClassDeclNode* curClassScope = getEnclosingClassNode();
          assert(curClassScope);

          ClassDeclNode* outer = cdn->getEnclosingClassNode();
          assert(outer);

          size_t n =
            curClassScope
              ->getClassSymbol()
              ->getOriginalUnliftedSymbol()
              ->getClassSymbolTable()
              ->countClassBoundaries(
                outer->getClassSymbol()->getClassSymbolTable());

          if (n == 0) {
            liftedParamExprs.push_back(new VariableSelfNode);
          } else {
            liftedParamExprs.push_back(createChainOfOuters(n));
          }
        }
      }

      // map each lifted param to a vector of names (possibly
      // adding the param to *this* scopes
      for (vector<BaseSymbol*>::const_iterator it = liftedParams.begin();
           it != liftedParams.end(); ++it) {
        VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
        Symbol* sym = static_cast<Symbol*>(*it);
        assert(sym->getDefinedSymbolTable() == ctx.definedIn);
        string paramName = ctx.refParamName(sym);
        liftedParamExprs.push_back(new VariableNodeParser(paramName, NULL));
      }

      size_t endPos = liftedParamExprs.size();
      liftedParamExprs.resize(endPos + args.size());

      // the regular params
      transform(args.begin(), args.end(),
                liftedParamExprs.begin() + endPos,
                ASTExpressionNode::CloneLiftFunctor(ctx));

      // replace calling the rewritten function
      return new FunctionCallNodeParser(
          primary->cloneForLift(ctx),
          TypeStringVec(),
          liftedParamExprs);
    }
  }
  return new FunctionCallNodeParser(
      primary->cloneForLift(ctx),
      TypeStringVec(),
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

FunctionCallNode*
FunctionCallNodeParser::cloneImpl(CloneMode::Type type) {
  switch (type) {
  case CloneMode::Structural:
    return new FunctionCallNodeParser(
        primary->clone(type),
        util::transform_vec(
          typeArgs.begin(), typeArgs.end(),
          ParameterizedTypeString::CloneFunctor()),
        util::transform_vec(
          args.begin(), args.end(),
          ASTExpressionNode::CloneFunctor(type)));
  case CloneMode::Semantic:
    assert(typeArgs.size() == typeArgTypes.size());
    return new FunctionCallNodeSynthetic(
        primary->clone(type),
        typeArgTypes,
        util::transform_vec(
          args.begin(), args.end(),
          ASTExpressionNode::CloneFunctor(type)));
  default: VENOM_NOT_REACHED;
  }
}

ASTExpressionNode*
FunctionCallNodeParser::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
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
FunctionCallNodeParser::print(ostream& o, size_t indent) {
  o << "(funccall-parser ";
  primary->print(o, indent);
  o << " ";
  if (!typeArgs.empty()) {
    o << "{";
    vector<string> strs =
      util::transform_vec(typeArgs.begin(), typeArgs.end(),
                          ParameterizedTypeString::StringerFunctor());
    o << util::join(strs.begin(), strs.end(), ", ");
    o << "} ";
  }
  PrintExprNodeVec(o, args, indent);
  o << ")";
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
