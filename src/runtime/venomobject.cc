#include <string>
#include <sstream>

#include <backend/vm.h>
#include <runtime/box.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>
#include <util/stl.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

ref_ptr<venom_object> venom_cell::asObject() const {
  return ref_ptr<venom_object>(data.obj);
}

void venom_cell::incRef() {
  if (data.obj) data.obj->incRef();
}

void venom_cell::decRef() {
  if (data.obj && !data.obj->decRef()) {
    delete data.obj;
  }
}

#ifndef NDEBUG
void venom_cell::AssertNonZeroRef(const venom_cell& cell) {
  assert(cell.asRawObject()->getCount());
}
#endif

venom_object* venom_object::Nil(NULL);
ref_ptr<venom_object> venom_object::NilPtr(NULL);

FunctionDescriptor* venom_object::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* venom_object::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* venom_object::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

venom_class_object venom_object::ObjClassTable(
    "object",
    sizeof(venom_object),
    0, 0,
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
      if (class_obj->ref_cell_bitmap & (0x1 << i)) p->decRef();
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
    venom_ret_cell str =
      f(NULL, venom_cell(const_cast<venom_object*>(this)));
    scoped_ret_value<venom_object> ptr(str.asRawObject());
    assert(ptr->getClassObj() == &venom_string::StringClassTable);
    return static_cast<venom_string*>(ptr.get())->getData();
  } else {
    stringstream buf;
    buf << "object@0x" << hex << intptr_t(this)
        << "(non-native stringify method)";
    return buf.str();
  }
}

venom_ret_cell
venom_object::stringify(ExecutionContext* ctx, venom_cell self) {
  venom_object_ptr ptr = self.asObject();
  stringstream buf;
  buf << "object@0x" << hex << intptr_t(self.asRawObject());
  return venom_ret_cell(new venom_string(buf.str()));
}

venom_ret_cell
venom_object::virtualDispatch(ExecutionContext* ctx, size_t index) {
  ctx->resumeExecution(this, index);
  venom_cell ret = ctx->program_stack.top();
  ctx->program_stack.pop();
  return venom_ret_cell(ret);
}

}
}
