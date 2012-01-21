#include <ast/expression/synthetic/functioncall.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

FunctionCallNode*
FunctionCallNodeSynthetic::cloneImpl() {
  return new FunctionCallNodeSynthetic(
      primary->clone(),
      typeArgTypes,
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneFunctor()));
}

ASTExpressionNode*
FunctionCallNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {
  // TODO: implement rewrite...
  return new FunctionCallNodeSynthetic(
      primary->cloneForLift(ctx),
      typeArgTypes,
      util::transform_vec(
        args.begin(), args.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

FunctionCallNode*
FunctionCallNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
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

}
}
