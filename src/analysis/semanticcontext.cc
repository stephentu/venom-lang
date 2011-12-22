#include <analysis/semanticcontext.h>

using namespace std;

namespace venom {
namespace analysis {

Type* SemanticContext::getType(const string& name) {
  map<std::string, Type*>::iterator it = types.find(name);
  return it == types.end() ? NULL : it->second;
}

Type* SemanticContext::createType(const string& name,
                                  Type* parent,
                                  size_t params /* = 0*/) {
  //TODO: check if type already exists
  Type* t = new Type(name, this, parent, params);
  types[name] = t;
  return t;
}

}
}
