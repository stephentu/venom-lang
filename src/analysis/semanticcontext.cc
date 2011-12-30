#include <algorithm>

#include <analysis/semanticcontext.h>
#include <ast/expression/node.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

Type* SemanticContext::createType(const string&     name,
                                  InstantiatedType* parent,
                                  size_t            params /* = 0*/) {
  Type* t = new Type(name, NULL, parent, params);
  types.push_back(t);
  return t;
}

Type* SemanticContext::createTypeParam(const string& name, size_t pos) {
  Type* t = new TypeParamType(name, pos);
  types.push_back(t);
  return t;
}

InstantiatedType*
SemanticContext::createInstantiatedType(
    Type* type, const vector<InstantiatedType*>& params) {
  InstantiatedType* t = new InstantiatedType(type, params);
  itypes.push_back(t);
  return t;
}

struct functor {
  functor(SemanticContext* ctx, SymbolTable* st) : ctx(ctx), st(st) {}
  inline InstantiatedType*
  operator()(const ParameterizedTypeString* t) const {
    return ctx->instantiateOrThrow(st, t);
  }
  SemanticContext *ctx;
  SymbolTable*     st;
};

InstantiatedType*
SemanticContext::instantiateOrThrow(SymbolTable *symbols,
                                    const ParameterizedTypeString* type) {

  TypeTranslator t;
  SymbolTable *cur = symbols;
  ClassSymbol *cs = NULL;
  for (vector<string>::const_iterator it = type->names.begin();
       it != type->names.end(); ++it) {
    cs = cur->findClassSymbol(*it, SymbolTable::AllowCurrentScope, t);
    if (!cs) {
      throw SemanticViolationException(
          "Type " + util::join(type->names.begin(), it + 1, ".") +
          " not defined");
    }
    if (it != type->names.end() - 1) {
      // TODO: fix this limitation
      // for now, only the outermost type can have type parameters
      if (cs->getType()->getParams()) {
        throw SemanticViolationException(
            "Implementation limitation: Cannot select inner class "
            "from parameterized class: " +
            util::join(type->names.begin(), it + 1, "."));
      }
    }
    cur = cs->getClassSymbolTable();
    assert(cur);
  }
  assert(cs);
  if (cs->getType()->getParams() != type->params.size()) {
      throw SemanticViolationException(
          "Wrong number of type parameters given to " +
          cs->getType()->getName());
  }

  vector<InstantiatedType*> buf(type->params.size());
  transform(type->params.begin(), type->params.end(),
            buf.begin(), functor(this, symbols));

  return t.translate(this, cs->getType()->instantiate(this, buf));
}

}
}
