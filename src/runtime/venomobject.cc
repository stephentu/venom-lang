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

venom_cell::venom_cell(const venom_cell& that)
  : data(that.data), type(that.type) {
  if (type == ObjType && data.obj) data.obj->incRef();
}

venom_cell::~venom_cell() {
  if (type == ObjType && data.obj && !data.obj->decRef()) delete data.obj;
}

venom_cell& venom_cell::operator=(const venom_cell& that) {
  // objects have to be treated specially
  bool thisIsObject = isObject();
  bool thatIsObject = that.isObject();
  if (thisIsObject) {
    // need to decRef and possibly free, but only if that doesn't contain the
    // same pointer
    if (thatIsObject && data.obj == that.data.obj) return *this;
    if (data.obj && !data.obj->decRef()) delete data.obj;
  }
  // now, regular assignment is ok
  data = that.data;
  type = that.type;
  // do an incRef if we are now an object
  if (type == ObjType && data.obj) data.obj->incRef();
  return *this;
}

venom_object* venom_object::Nil(NULL);

}
}
