#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/statement/synthetic/funcdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

FuncDeclNode*
FuncDeclNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new FuncDeclNodeSynthetic(
    name,
    typeParamTypes,
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneFunctor(type)),
    retType,
    stmts->clone(type));
}

ASTStatementNode*
FuncDeclNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {
  return new FuncDeclNodeSynthetic(
    name,
    typeParamTypes,
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneLiftFunctor(ctx)),
    retType,
    stmts->cloneForLift(ctx));
}

FuncDeclNode*
FuncDeclNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  // TODO: assert translator doesn't change this type
  return new FuncDeclNodeSynthetic(
    name,
    typeParamTypes,
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneTemplateFunctor(t)),
    retType,
    stmts->cloneForTemplate(t));
}

void
FuncDeclNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(func " << name << " -> " << retType->stringify();
  o << std::endl << util::indent(indent + 1);
  o << "(params ";
  PrintExprNodeVec(o, params, indent + 1);
  o << ")";
  o << std::endl << util::indent(indent + 1);
  stmts->print(o, indent + 1);
  o << ")";
}

void
FuncDeclNodeSynthetic::checkAndInitTypeParams(SemanticContext* ctx) {
  for (size_t pos = 0; pos < typeParamTypes.size(); pos++) {
    // add all the type params into the body's symtab
    VENOM_ASSERT_TYPEOF_PTR(TypeParamType, typeParamTypes[pos]->getType());
    stmts->getSymbolTable()->createClassSymbol(
        typeParamTypes[pos]->getType()->getName(),
        ctx->getRootSymbolTable()->newChildScope(NULL),
        typeParamTypes[pos]->getType());
  }
}

void
FuncDeclNodeSynthetic::checkAndInitReturnType(SemanticContext* ctx) {}

}
}
