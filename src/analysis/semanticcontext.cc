#include <algorithm>

#include <analysis/semanticcontext.h>
#include <ast/expression/node.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

Type* SemanticContext::createType(const string& name,
                                  Type* parent,
                                  size_t params /* = 0*/) {
  //TODO: check if type already exists
  Type* t = new Type(name, this, parent, params);
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
  ClassSymbol *cs = symbols->findClassSymbol(type->name, true);
  if (!cs) {
    throw SemanticViolationException(
        "Type " + type->name + " not defined");
  }
  if (cs->getType()->getParams() != type->params.size()) {
      throw SemanticViolationException(
          "Wrong number of type parameters given to " + type->name);
  }

  vector<InstantiatedType*> buf;
  buf.resize(type->params.size());
  transform(type->params.begin(), type->params.end(),
            buf.begin(), functor(this, symbols));

  return cs->getType()->instantiate(this, buf);
}

}
}
