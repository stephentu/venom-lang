#include <algorithm>
#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>
#include <analysis/typetranslator.h>

#include <util/stl.h>

using namespace std;

namespace venom {
namespace analysis {

struct find_functor {
  find_functor(InstantiatedType* type) : type(type) {}
  inline bool operator()(const InstantiatedTypePair& p) const {
    return p.first->equals(*type);
  }
  InstantiatedType* type;
};

InstantiatedType*
TypeTranslator::translate(SemanticContext* ctx, InstantiatedType* type) const {
  TypeMap::const_iterator it =
    find_if(map.begin(), map.end(), find_functor(type));
  if (it != map.end()) return it->second;
  vector<InstantiatedType*> buf(type->getParams().size());
  transform(type->getParams().begin(), type->getParams().end(),
            buf.begin(), TypeTranslator::TranslateFunctor(ctx, this));
  return ctx->createInstantiatedType(type->getType(), buf);
}

void
TypeTranslator::bind(InstantiatedType* type) {
  vector<InstantiatedType*> &lhs =
    type->getType()->getClassSymbol()->getTypeParams();
  vector<InstantiatedType*> &rhs = type->getParams();
  assert(lhs.size() == rhs.size());
  TypeMap tmap(lhs.size());
  util::zip(lhs.begin(), lhs.end(), rhs.begin(), tmap.begin());
  map.insert(map.end(), tmap.begin(), tmap.end());
}

}
}
