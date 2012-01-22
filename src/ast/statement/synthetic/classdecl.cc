#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/synthetic/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassDeclNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(class " << name << std::endl << util::indent(indent + 1);
  vector<string> typenames(typeParamTypes.size());
  transform(typeParamTypes.begin(), typeParamTypes.end(),
            typenames.begin(),
            util::stringify_functor<InstantiatedType>::ptr());
  o << "(type-params (" <<
    util::join(typenames.begin(), typenames.end(), ",") <<
    "))" << std::endl << util::indent(indent + 1);
  stmts->print(o, indent + 1);
  o << ")";
}

void
ClassDeclNodeSynthetic::checkAndInitTypeParams(SemanticContext* ctx) {
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
ClassDeclNodeSynthetic::checkAndInitParents(SemanticContext* ctx) {}

void
ClassDeclNodeSynthetic::createClassSymbol(
    const string& name,
    SymbolTable* classTable,
    Type* type,
    const vector<InstantiatedType*>& typeParams) {
  if (instantiation) {
    assert(typeParams.empty());
    symbols->createSpecializedClassSymbol(
        classTable, instantiation, type);
  } else {
    ClassDeclNode::createClassSymbol(name, classTable, type, typeParams);
  }
}

ClassDeclNode*
ClassDeclNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new ClassDeclNodeSynthetic(
      name,
      parentTypes,
      typeParamTypes,
      stmts->clone(type));
}

ASTStatementNode*
ClassDeclNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {
  return new ClassDeclNodeSynthetic(
      name,
      parentTypes,
      typeParamTypes,
      stmts->cloneForLift(ctx));
}

ClassDeclNode*
ClassDeclNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  // TODO: assert that the TypeTranslator doesn't instantiate this
  // class type
  return new ClassDeclNodeSynthetic(
      name,
      parentTypes,
      typeParamTypes,
      stmts->cloneForTemplate(t));
}

}
}
