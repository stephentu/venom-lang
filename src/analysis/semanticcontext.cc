#include <algorithm>

#include <analysis/semanticcontext.h>
#include <ast/expression/node.h>
#include <ast/statement/node.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

SemanticContext::~SemanticContext() {
  // we have ownership of moduleRoot + children + types + rootSymbols
  if (moduleRoot) delete moduleRoot;
  util::delete_pointers(children.begin(), children.end());
  util::delete_pointers(types.begin(), types.end());
  if (!parent) {
    delete rootSymbols;
    Type::ResetBuiltinTypes();
  }
}

SemanticContext* SemanticContext::findModule(const util::StrVec& names) {
  if (names.empty()) return this;
  map<string, SemanticContext*>::iterator it = childrenMap.find(names[0]);
  if (it == childrenMap.end()) return NULL;
  // TODO: might want to use iterators for this, internally
  return it->second->findModule(util::StrVec(names.begin() + 1, names.end()));
}

SemanticContext* SemanticContext::createModule(const util::StrVec& names) {
  if (names.empty()) return this;
  map<string, SemanticContext*>::iterator it = childrenMap.find(names[0]);
  if (it == childrenMap.end()) {
    // make a new child
    SemanticContext *child = newChildContext(names[0]);
    return child->createModule(util::StrVec(names.begin() + 1, names.end()));
  }
  // TODO: might want to use iterators for this, internally
  return it->second->createModule(
      util::StrVec(names.begin() + 1, names.end()));
}

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
