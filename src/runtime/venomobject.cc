#include <string>
#include <sstream>

#include <backend/vm.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>
#include <util/stl.h>

using namespace std;
using namespace venom::backend;

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

string venom_cell::stringifyNativeOnly() const {
  assert(isInitialized());
  switch (type) {
  case IntType: return util::stringify(data.int_value);
  case DoubleType: return util::stringify(data.double_value);
  case BoolType: return data.bool_value ? "True" : "False";
  case ObjType: return !data.obj ? "Nil" : data.obj->stringifyNativeOnly();
  default: VENOM_NOT_REACHED;
  }
}

venom_object* venom_object::Nil(NULL);
ref_ptr<venom_object> venom_object::NilPtr(NULL);

FunctionDescriptor* venom_object::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, true));

FunctionDescriptor* venom_object::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, true));

FunctionDescriptor* venom_object::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, true));

venom_class_object venom_object::ObjClassTable(
    "object",
    sizeof(venom_object),
    0,
    util::vec3(InitDescriptor, ReleaseDescriptor, StringifyDescriptor));

/**
 * We cannot have venom_object() in the header since we have not seen
 * a complete declaration of ExecutionContext
 */
venom_object::venom_object(venom_class_object* class_obj)
  : class_obj(class_obj) {
  assert(class_obj);

  // this is equivalent to looping over each of the
  // cells and calling the constructor, but faster
  if (class_obj->n_cells) {
    memset(cell_ptr(0), 0, class_obj->n_cells * sizeof(venom_cell));
  }

  assert(!count);
  scoped_ref_counter<venom_object> helper(this);
  assert(count == 1);
  // simulate calling the class constructor
  // we *must* bump the ref count here, so that we don't end up destructing the
  // object when virtualDispatch is finished
  virtualDispatch(ExecutionContext::current_context(), 0);
  assert(count == 1);
}

/**
 * We cannot have ~venom_object() in the header since we have not seen
 * a complete declaration of ExecutionContext
 */
venom_object::~venom_object() {
  assert(!count);
  scoped_ref_counter<venom_object> helper(this);
  assert(count == 1);
  // simulate virtual destructor
  // we *must* bump the ref count here, so that we don't end up destructing the
  // object again (infinitely) when virtualDispatch is finished
  virtualDispatch(ExecutionContext::current_context(), 1);
  assert(count == 1);

  // destroy the cells
  if (class_obj->n_cells) {
    venom_cell *p = cell_ptr(0);
    // manually call the destructor. we *don't* delete
    // a venom_cell, since we did not allocate it with
    // operator new.
    for (size_t i = 0; i < class_obj->n_cells; i++, p++) {
      p->~venom_cell();
    }
  }
}

string venom_object::stringifyNativeOnly() const {
  FunctionDescriptor *desc = class_obj->vtable[2];
  assert(desc);
  if (desc->isNative()) {
    assert(desc->getNumArgs() == 1);
    FunctionDescriptor::F1 f =
      reinterpret_cast<FunctionDescriptor::F1>(desc->getFunctionPtr());
    venom_object_ptr str =
      f(NULL, venom_object_ptr(const_cast<venom_object*>(this)));
    assert(str->getClassObj() == &venom_string::StringClassTable);
    return static_cast<venom_string*>(str.get())->getData();
  } else {
    stringstream buf;
    buf << "object@0x" << hex << intptr_t(this)
        << "(non-native stringify method)";
    return buf.str();
  }
}

ref_ptr<venom_object>
venom_object::stringify(ExecutionContext* ctx, ref_ptr<venom_object> self) {
  stringstream buf;
  buf << "object@0x" << hex << intptr_t(self.get());
  return ref_ptr<venom_object>(new venom_string(buf.str()));
}

ref_ptr<venom_object>
venom_object::virtualDispatch(ExecutionContext* ctx, size_t index) {
  ctx->resumeExecution(this, index);
  venom_cell ret = ctx->program_stack.top();
  // TODO: box instead of asserting object
  assert(ret.isObject());
  return ref_ptr<venom_object>(ret.asRawObject());
}

}
}
