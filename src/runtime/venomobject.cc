/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
void venom_cell::AssertNonZeroRefCount(const venom_cell& cell) {
  assert(!cell.asRawObject() || cell.asRawObject()->getCount());
}
#endif

venom_object* venom_object::Nil(NULL);

// This uses the "construct-on-first-use" idiom, which
// is supposed to be thread safe at least in GCC (see
// https://arkaitzj.wordpress.com/2009/11/07/static-locals-and-threadsafety-in-g/).
//
// TODO: we should w/ mutex for non-gcc compilers

FunctionDescriptor& venom_object::InitDescriptor() {
  static FunctionDescriptor f((void*)init, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_object::ReleaseDescriptor() {
  static FunctionDescriptor f((void*)release, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_object::CtorDescriptor() {
  static FunctionDescriptor f((void*)ctor, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_object::StringifyDescriptor() {
  static FunctionDescriptor f((void*)stringify, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_object::HashDescriptor() {
  static FunctionDescriptor f((void*)hash, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_object::EqDescriptor() {
  static FunctionDescriptor f((void*)eq, 2, 0x3, true);
  return f;
}

venom_class_object& venom_object::ObjClassTable() {
  static venom_class_object c(
    "object",
    sizeof(venom_object),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

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
  dispatchInit(ExecutionContext::current_context());
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
  dispatchRelease(ExecutionContext::current_context());
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
  FunctionDescriptor *desc = class_obj->vtable[0];
  assert(desc);
  if (desc->isNative()) {
    assert(desc->getNumArgs() == 1);
    FunctionDescriptor::F1 f =
      reinterpret_cast<FunctionDescriptor::F1>(desc->getFunctionPtr());
    venom_ret_cell str =
      f(NULL, venom_cell(const_cast<venom_object*>(this)));
    scoped_ret_value<venom_object> ptr(str.asRawObject());
    assert(ptr->getClassObj() == &venom_string::StringClassTable());
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
venom_object::virtualDispatch(
    ExecutionContext* ctx,
    size_t index,
    const vector<venom_cell>& args) {

  FunctionDescriptor *desc = getClassObj()->vtable.at(index);

#ifndef NDEBUG
  assert(desc->getNumArgs() >= 1); // since its a method
  // getNumArgs includes "this" pointer
  assert(desc->getNumArgs() - 1 == args.size());
#endif /* NDEBUG */

  // push arguments in reverse order
  for (ssize_t idx = args.size() - 1; idx >= 0; idx--) {
    // see if we need to incRef
    venom_cell arg = args[idx];
    if (desc->argRefCellBitmap() & (0x1UL << (idx + 1))) {
      arg.incRef();
    }
    ctx->program_stack.push(arg);
  }

  ctx->resumeExecution(this, desc);
  venom_cell ret = ctx->program_stack.top();
  ctx->program_stack.pop();
  return venom_ret_cell(ret);
}

venom_ret_cell
venom_object::dispatchInit(ExecutionContext* ctx) {
  ctx->resumeExecution(this, class_obj->cppInit);
  venom_cell ret = ctx->program_stack.top();
  ctx->program_stack.pop();
  return venom_ret_cell(ret);
}

venom_ret_cell
venom_object::dispatchRelease(ExecutionContext* ctx) {
  ctx->resumeExecution(this, class_obj->cppRelease);
  venom_cell ret = ctx->program_stack.top();
  ctx->program_stack.pop();
  return venom_ret_cell(ret);
}

}
}
