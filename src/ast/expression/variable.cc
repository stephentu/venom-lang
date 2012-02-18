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

#include <ast/expression/attraccess.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/symbolnode.h>
#include <ast/expression/synthetic/variable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
VariableNode::registerSymbol(SemanticContext* ctx) {
  assert(!symbol);
  symbol = symbols->findBaseSymbol(
      name, SymbolTable::Any, SymbolTable::AllowCurrentScope, translator);
  if (!symbol) {
    throw SemanticViolationException(
        "Symbol " + name + " is not defined in scope");
  }
}

void
VariableNode::collectNonLocalRefs(LiftContext& ctx) {
  Symbol *s;
  MethodSymbol* m;
  if (isNonLocalRef(ctx.definedIn, s, m) && s && !s->isObjectField()) {
    bool create;
    ctx.refs.create(s, create);
  }
}

InstantiatedType*
VariableNode::typeCheckImpl(SemanticContext* ctx,
                            InstantiatedType* expected,
                            const InstantiatedTypeVec& typeParamArgs) {
  assert(symbol);
  return symbol->bind(ctx, translator, typeParamArgs);
}

bool
VariableNode::isNonLocalRef(SymbolTable* definedIn,
                            Symbol*& nonLocalSym,
                            MethodSymbol*& nonLocalMeth) {
  BaseSymbol* bsym = symbol;
  if (Symbol* sym = dynamic_cast<Symbol*>(bsym)) {
    if (sym->getDefinedSymbolTable() == definedIn) {
      nonLocalSym = sym;
      nonLocalMeth = NULL;
      return true;
    }
  }
  if (MethodSymbol* meth = dynamic_cast<MethodSymbol*>(bsym)) {
    if (meth->getDefinedSymbolTable() == definedIn) {
      nonLocalSym = NULL;
      nonLocalMeth = meth;
      return true;
    }
  }
  nonLocalSym = NULL;
  nonLocalMeth = NULL;
  return false;
}

ASTNode*
VariableNode::rewriteAfterLift(const LiftContext::LiftMap& liftMap,
                               const set<BaseSymbol*>& refs) {
  BaseSymbol* bs = symbol;
  // self/super nodes do not have symbols...
  if (!bs) return NULL;
  set<BaseSymbol*>::const_iterator it = refs.find(bs);
  if (it == refs.end()) return NULL;
  VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
  Symbol* sym = static_cast<Symbol*>(bs);
  VENOM_ASSERT_NOT_NULL(sym);
  assert(sym->getDefinedSymbolTable() == symbols);
  assert(sym->isPromoteToRef());
  return replace(
      getSymbolTable()->getSemanticContext(),
      new AttrAccessNode(
        new VariableNodeParser(name, NULL), "value"));
}

ASTNode*
VariableNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  if (mode != CanonicalRefs) {
    return ASTExpressionNode::rewriteLocal(ctx, mode);
  }

  BaseSymbol *bs = symbol;
  assert(bs); // TODO: why is this assert ok?
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    if (sym->isModuleLevelSymbol()) {
      assert(!sym->isObjectField());

      // need to rewrite x to reference x off of the
      // module object
      ModuleSymbol *msym =
        ctx->getRootSymbolTable()->findModuleSymbol(
            ctx->getModuleName(), SymbolTable::NoRecurse);
      assert(msym);
      AttrAccessNode *rep = new AttrAccessNode(new SymbolNode(msym), name);
      return replace(ctx, rep);
    }

    if (sym->isObjectField()) {
      // rewrite x into self.x
      AttrAccessNode *rep = new AttrAccessNode(new VariableSelfNode, name);
      return replace(ctx, rep);
    }
  } else if (dynamic_cast<MethodSymbol*>(bs)) {
    AttrAccessNode *rep = new AttrAccessNode(new VariableSelfNode, name);
    return replace(ctx, rep);
  }
  return NULL;
}

void
VariableNode::codeGen(CodeGenerator& cg) {
  BaseSymbol *bs = symbol;
  assert(bs);
  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    bool create;
    size_t idx = cg.createLocalVariable(sym, create);
    assert(!create);
    cg.emitInstU32(
        getStaticType()->isRefCounted() ?
          Instruction::LOAD_LOCAL_VAR_REF : Instruction::LOAD_LOCAL_VAR,
        idx);
  } else {
    // otherwise if we are referencing a module/class,
    // then wait until the AttrAccess node to generate
    // code
  }
}

InstantiatedType*
VariableNodeParser::getExplicitType() {
  if (explicitTypeString && !explicitType) {
    explicitType =
      symbols->getSemanticContext()->instantiateOrThrow(
          symbols, explicitTypeString);
  }
  assert(bool(explicitTypeString) == bool(explicitType));
  return explicitType;
}

VariableNode*
VariableNodeParser::cloneImpl(CloneMode::Type type) {
  switch (type) {
  case CloneMode::Structural:
    return new VariableNodeParser(
        name, explicitTypeString ? explicitTypeString->clone() : NULL);
  case CloneMode::Semantic:
    assert(symbol);
    return new SymbolNode(symbol, translator, getExplicitType());
  default: VENOM_NOT_REACHED;
  }
}

ASTExpressionNode*
VariableNodeParser::cloneForLiftImpl(LiftContext& ctx) {
  Symbol* s;
  MethodSymbol* m;
  if (isNonLocalRef(ctx.definedIn, s, m)) {
    if (s && !s->isObjectField()) {
      // case 1: non-local stack variable
      assert(!explicitTypeString);
      assert(!explicitType);
      string newName = ctx.refParamName(s);
      return new AttrAccessNode(
          new VariableNodeParser(newName, NULL), "value");
    }
    if ((s && s->isObjectField()) || m) {
      // case 2: non-local class member

      // calculate how many <outer> deferences we need. it is equal to the
      // number of class boundaries crossed when going from the *original*
      // (non-lifted) scope to the symbol's scope

      size_t n = symbols->countClassBoundaries(ctx.definedIn);
      assert(n > 0);
      return ASTNode::CreateOuterAttrChain(n, name);
    }
  }

  // case 3: reference to the scope being lifted
  BaseSymbol* bs = symbol;
  if (bs == ctx.curLiftSym) {
    return new VariableNodeParser(ctx.liftedName, NULL);
  }

  LiftContext::LiftMap::const_iterator it =
    ctx.liftMap.find(bs);
  if (it != ctx.liftMap.end()) {
    ASTStatementNode* liftedStmt = it->second.second;
    string liftedName;
    // TODO: make getName() virtual function on ASTNode
    if (FuncDeclNode* fdn = dynamic_cast<FuncDeclNode*>(liftedStmt)) {
      liftedName = fdn->getName();
    } else if (ClassDeclNode* cdn = dynamic_cast<ClassDeclNode*>(liftedStmt)) {
      liftedName = cdn->getName();
    } else VENOM_NOT_REACHED;
    return new VariableNodeParser(liftedName, NULL);
  }

  SemanticContext* sctx = symbols->getSemanticContext();

  // need to possibly retype, if the type is a lifted class
  if (explicitType) {
    LiftContext::LiftMap::const_iterator it =
      ctx.liftMap.find(explicitType->findSpecializedClassSymbol());
    if (it != ctx.liftMap.end()) {
      BaseSymbol* bs = it->second.second->getSymbol();
      VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, bs);
      ClassSymbol* cs = static_cast<ClassSymbol*>(bs);

      // OK to use InstantiatedType object, because we use
      // the type of the lifted class (type won't change after clone)
      return new VariableNodeSynthetic(name, cs->getSelfType(sctx));
    }

    if (ctx.isLiftingClass() &&
        ctx.useExplicitTypeForClassSymbol(explicitType->getClassSymbol())) {
      return new VariableNodeSynthetic(name, explicitType);
    }

    return new VariableNodeParser(
        name,
        explicitType

          // WARNING: calling findCodeGeneratableIType()
          // causes several test regressions...
          //->findCodeGeneratableIType(sctx)

          ->toParameterizedString(ctx.liftInto));
  }

  return new VariableNodeParser(name, NULL);
}

VariableNode*
VariableNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  InstantiatedType* itype = getExplicitType();
  if (itype) {
    assert(explicitTypeString);
    SemanticContext* ctx = getSymbolTable()->getSemanticContext();
    itype = t.translate(ctx, itype);
    ASTStatementNode* enclosing = getEnclosingTypeParameterizedNode();
    assert(enclosing);
    return new VariableNodeParser(
        name, itype->toParameterizedString(enclosing->getSymbolTable()));
  }
  return new VariableNodeParser(name, NULL);
}

void
VariableSelfNode::registerSymbol(SemanticContext* ctx) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  if (!cdn) {
    throw SemanticViolationException(
        "self cannot be used outside of class scope");
  }
}

InstantiatedType*
VariableSelfNode::typeCheckImpl(SemanticContext* ctx,
                                InstantiatedType* expected,
                                const InstantiatedTypeVec& typeParamArgs) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  assert(cdn);
  TypeTranslator t;
  ClassSymbol *cs =
    cdn->getSymbolTable()->findClassSymbol(
        cdn->getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getSelfType(ctx);
}

void
VariableSelfNode::codeGen(CodeGenerator& cg) {
  // self is always slot 0
  cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, 0);
}

VariableSelfNode*
VariableSelfNode::cloneImpl(CloneMode::Type type) {
  return new VariableSelfNode;
}

ASTExpressionNode*
VariableSelfNode::cloneForLiftImpl(LiftContext& ctx) {
  return new VariableSelfNode;
}

VariableSelfNode*
VariableSelfNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new VariableSelfNode;
}

void
VariableSuperNode::registerSymbol(SemanticContext* ctx) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  if (!cdn) {
    throw SemanticViolationException(
        "super cannot be used outside of class scope");
  }
}

InstantiatedType*
VariableSuperNode::typeCheckImpl(SemanticContext* ctx,
                                 InstantiatedType* expected,
                                 const InstantiatedTypeVec& typeParamArgs) {
  ClassDeclNode *cdn = getEnclosingClassNode();
  assert(cdn);
  TypeTranslator t;
  ClassSymbol *cs =
    cdn->getSymbolTable()->findClassSymbol(
        cdn->getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getType()->getParent();
}

void
VariableSuperNode::codeGen(CodeGenerator& cg) {
  // super (self) is always slot 0
  cg.emitInstU32(Instruction::LOAD_LOCAL_VAR_REF, 0);
}

VariableSuperNode*
VariableSuperNode::cloneImpl(CloneMode::Type type) {
  return new VariableSuperNode;
}

ASTExpressionNode*
VariableSuperNode::cloneForLiftImpl(LiftContext& ctx) {
  return new VariableSuperNode;
}

VariableSuperNode*
VariableSuperNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new VariableSuperNode;
}

}
}
