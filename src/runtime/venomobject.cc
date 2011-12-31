#include <runtime/venomobject.h>

using namespace std;

namespace venom {
namespace runtime {

// These ctors/dtors cannot be placed in the header, because
// we only have a forward decl of venom_object (so we can't
// incRef()/decRef() it)

venom_cell::venom_cell(venom_object* obj) : data(obj), type(ObjType) {
  if (obj) obj->incRef();
}

venom_cell::~venom_cell() {
  if (type == ObjType && data.obj && !data.obj->decRef()) delete data.obj;
}

}
}
