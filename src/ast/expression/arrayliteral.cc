#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayliteral.h>
#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

ASTNode*
ArrayLiteralNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  // recurse first
  ASTExpressionNode::rewriteLocal(ctx, mode);
  if (mode != DeSugar) return NULL;

  // now, turn it from:
  // [expr1, expr2, ..., exprN]
  //
  // to:
  // (_tmp = list{type}(), _tmp.append(expr1), ..., _tmp.append(exprN), _tmp)

  InstantiatedType* stype = getStaticType();
  assert(stype->getType()->isListType());
  string tmpVar = ctx->tempVarName();
  ExprNodeVec exprs;
  exprs.reserve(values.size() + 2);
  exprs.push_back(
      new AssignExprNode(
        new VariableNodeParser(tmpVar, NULL),
        new FunctionCallNodeSynthetic(
          new SymbolNode(
            stype->getType()->getClassSymbol()),
          stype->getParams(),
          ExprNodeVec())));
  for (ExprNodeVec::iterator it = values.begin();
       it != values.end(); ++it) {
    exprs.push_back(
        new FunctionCallNodeParser(
          new AttrAccessNode(
            new VariableNodeParser(tmpVar, NULL),
            "append"),
          TypeStringVec(),
          util::vec1((*it)->clone(CloneMode::Semantic))));
  }
  exprs.push_back(new VariableNodeParser(tmpVar, NULL));
  return replace(ctx, new ExprListNode(exprs));
}

struct functor {
  functor(SemanticContext* ctx) : ctx(ctx) {}
  inline InstantiatedType* operator()(ASTExpressionNode* exp) const {
    return exp->typeCheck(ctx);
  }
  SemanticContext* ctx;
};

struct reduce_functor_t {
  inline InstantiatedType*
  operator()(InstantiatedType* accum, InstantiatedType* cur) const {
    return accum->mostCommonType(cur);
  }
} reduce_functor;

InstantiatedType*
ArrayLiteralNode::typeCheckImpl(SemanticContext* ctx,
                                InstantiatedType* expected,
                                const InstantiatedTypeVec& typeParamArgs) {
  if (values.empty()) {
    if (expected && expected->getType()->equals(*Type::ListType)) {
      return expected;
    }
    return Type::ListType->instantiate(
        ctx, util::vec1(InstantiatedType::AnyType));
  } else {
    vector<InstantiatedType*> types(values.size());
    transform(values.begin(), values.end(), types.begin(),
              functor(ctx));
    return Type::ListType->instantiate(
        ctx, util::vec1(
          util::reducel(types.begin(), types.end(), reduce_functor)));
  }
}

ArrayLiteralNode*
ArrayLiteralNode::cloneImpl(CloneMode::Type type) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneFunctor(type)));
}

ASTExpressionNode*
ArrayLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

ArrayLiteralNode*
ArrayLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)));
}

}
}
