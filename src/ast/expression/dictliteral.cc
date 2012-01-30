#include <cassert>
#include <utility>

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/dictliteral.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <backend/codegenerator.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

typedef pair<InstantiatedType*, InstantiatedType*> ITypePair;

struct fold_functor {
  fold_functor(SemanticContext* ctx) : ctx(ctx) {}
  inline ITypePair
  operator()(const ITypePair& accum, DictPair* cur) const {
    if (!accum.first) {
      assert(!accum.second);
      return ITypePair(
          cur->key()->typeCheck(ctx),
          cur->value()->typeCheck(ctx));
    } else {
      assert(accum.second);
      return ITypePair(
          accum.first->mostCommonType(cur->key()->typeCheck(ctx)),
          accum.second->mostCommonType(cur->value()->typeCheck(ctx)));
    }
  }
  SemanticContext* ctx;
};

DictPair*
DictPair::cloneImpl(CloneMode::Type type) {
  return new DictPair(first->clone(type), second->clone(type));
}

ASTExpressionNode*
DictPair::cloneForLiftImpl(LiftContext& ctx) {
  return new DictPair(first->cloneForLift(ctx), second->cloneForLift(ctx));
}

DictPair*
DictPair::cloneForTemplateImpl(const TypeTranslator& t) {
  return new DictPair(first->cloneForTemplate(t), second->cloneForTemplate(t));
}

InstantiatedType*
DictLiteralNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {
  if (pairs.empty()) {
    if (expected && expected->getType()->equals(*Type::MapType)) {
      return expected;
    }
    return Type::MapType->instantiate(
        ctx, util::vec2(InstantiatedType::AnyType, InstantiatedType::AnyType));
  } else {
    ITypePair res =
      util::foldl(pairs.begin(), pairs.end(),
                  ITypePair(NULL, NULL), fold_functor(ctx));
    return Type::MapType->instantiate(
        ctx, util::vec2(res.first, res.second));
  }
}

ASTNode*
DictLiteralNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  // recurse first
  ASTExpressionNode::rewriteLocal(ctx, mode);
  if (mode != DeSugar) return NULL;

  // turn it from:
  // { k1 : v1, k2 : v2, ..., kN : vN]
  //
  // to:
  // (_tmp = map{k, v}(), _tmp.set(k1, v1), ..., _tmp.set(kN, vN), _tmp)

  InstantiatedType* stype = getStaticType();
  assert(stype->getType()->isMapType());
  string tmpVar = ctx->tempVarName();
  ExprNodeVec exprs;
  exprs.reserve(pairs.size() + 2);
  exprs.push_back(
      new AssignExprNode(
        new VariableNodeParser(tmpVar, NULL),
        new FunctionCallNodeSynthetic(
          new SymbolNode(
            stype->getType()->getClassSymbol()),
          stype->getParams(),
          ExprNodeVec())));
  for (DictPairVec::iterator it = pairs.begin();
       it != pairs.end(); ++it) {
    exprs.push_back(
        new FunctionCallNodeParser(
          new AttrAccessNode(
            new VariableNodeParser(tmpVar, NULL),
            "set"),
          TypeStringVec(),
          util::vec2((*it)->key()->clone(CloneMode::Semantic),
                     (*it)->value()->clone(CloneMode::Semantic))));
  }
  exprs.push_back(new VariableNodeParser(tmpVar, NULL));
  return replace(ctx, new ExprListNode(exprs));
}

DictLiteralNode*
DictLiteralNode::cloneImpl(CloneMode::Type type) {
  return new DictLiteralNode(
      util::transform_vec(
        pairs.begin(), pairs.end(), DictPair::CloneFunctor(type)));
}

ASTExpressionNode*
DictLiteralNode::cloneForLiftImpl(LiftContext& ctx) {

  ExprNodeVec clonedExprs =
    util::transform_vec(
            pairs.begin(), pairs.end(), DictPair::CloneLiftFunctor(ctx));
  DictPairVec cloned(clonedExprs.size());
  transform(clonedExprs.begin(), clonedExprs.end(), cloned.begin(),
            util::poly_ptr_cast_functor<ASTExpressionNode, DictPair>::checked());
  return new DictLiteralNode(cloned);
}

DictLiteralNode*
DictLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new DictLiteralNode(
      util::transform_vec(
        pairs.begin(), pairs.end(), DictPair::CloneTemplateFunctor(t)));
}

}
}
