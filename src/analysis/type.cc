#include <analysis/semanticcontext.h>
#include <analysis/type.h>

using namespace std;

namespace venom {
namespace analysis {

Type::~Type() {
  if (itype) delete itype;
}

InstantiatedType*
Type::instantiate(SemanticContext* ctx) {
  return instantiate(ctx, vector<InstantiatedType*>());
}

InstantiatedType*
Type::instantiate(SemanticContext* ctx,
                  const vector<InstantiatedType*>& params) {
  if (this->params != params.size()) return NULL;
  if (this->params == 0) {
    return itype ? itype : (itype = new InstantiatedType(this));
  } else {
    return ctx->createInstantiatedType(this, params);
  }
}

}
}
