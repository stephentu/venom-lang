#include <algorithm>

#include <ast/statement/funcdecl.h>
#include <ast/expression/variable.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::util;

namespace venom {
namespace ast {

struct name_functor_t {
  inline string operator()(ASTExpressionNode* node) const {
    return dynamic_cast<VariableNode*>(node)->getName();
  }
} name_functor;

struct itype_functor {
  itype_functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline InstantiatedType* operator()(ASTExpressionNode* node) const {
    VariableNode *vn = dynamic_cast<VariableNode*>(node);
    assert(vn->getExplicitParameterizedTypeString());
    return ctx->instantiateOrThrow(
        st, vn->getExplicitParameterizedTypeString());
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

void FuncDeclNode::registerSymbol(SemanticContext* ctx) {
  if (symbols->isDefined(name, SymbolTable::Any, false)) {
    throw SemanticViolationException(
        "Function/Method " + name + " already defined");
  }

  // check duplicate param names
  vector<string> names;
  names.resize(params.size());
  transform(params.begin(), params.end(), names.begin(), name_functor);
  if (!is_unique(names.begin(), names.end())) {
    throw SemanticViolationException("Duplicate parameter names");
  }

  // check and instantiate parameter types
  vector<InstantiatedType*> itypes;
  itypes.resize(params.size());
  transform(params.begin(), params.end(),
            itypes.begin(), itype_functor(ctx, symbols));

  // check and instantiate return type
  InstantiatedType* retType;
  if (ret_typename) {
    retType = ctx->instantiateOrThrow(symbols, ret_typename);
  } else {
    // treat no ret type as void type
    retType =
      ctx
        ->getRootSymbolTable()
        ->findClassSymbol("void", false)
        ->getType()
        ->instantiate(ctx);
  }

  // add symbol to current symtab
  symbols->createFuncSymbol(name, itypes, retType);

  // add parameters to block (child) symtab
  for (size_t i = 0; i < params.size(); i++) {
    VariableNode *vn = dynamic_cast<VariableNode*>(params[i]);
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), itypes[i]);
  }
}

}
}
