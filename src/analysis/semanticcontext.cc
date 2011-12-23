#include <analysis/semanticcontext.h>

using namespace std;

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

}
}
