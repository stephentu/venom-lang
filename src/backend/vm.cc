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

#include <algorithm>

#include <backend/vm.h>
#include <runtime/venomstring.h>
#include <util/scopehelpers.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

void ExecutionContext::execute(Callback& callback) {
  assert(program_counter);
  assert(*program_counter);
  util::ScopedBoolean sb(is_executing);
  util::ScopedVariable<ExecutionContext*> sv(_current, this);
  scoped_constants sc(this);

  primeStacks();
  new_frame(NULL); // denotes when <main> returns
  while (true) {
    if (VENOM_UNLIKELY((*program_counter)->execute(*this))) program_counter++;
    if (VENOM_UNLIKELY(program_counter == NULL)) {
      // end of stream
      assert(local_variables_stack.empty());
      assert(local_variables_ref_info_stack.empty());
      assert(frame_offset.empty());
      assert(ret_addr_stack.empty());
      callback.noResult();
      return;
    }
  }
}

struct const_init_functor {
  inline venom_cell* operator()(const ExecConstant& konst) const {
    if (konst.isLeft()) {
      venom_string *sptr = new venom_string(konst.left());
      sptr->incRef();
      return new venom_cell(sptr);
    } else {
      venom_class_object* class_obj = konst.right();
      venom_object* obj = venom_object::allocObj(class_obj);
      obj->incRef();
      return new venom_cell(obj);
    }
  }
};

void ExecutionContext::initConstants() {
  assert(!constant_pool);
  constant_pool = new venom_cell* [code->constant_pool.size()];
  transform(code->constant_pool.begin(), code->constant_pool.end(),
            constant_pool, const_init_functor());
}

void ExecutionContext::releaseConstants() {
  assert(constant_pool);
  for (size_t i = 0; i < code->constant_pool.size(); i++) {
    constant_pool[i]->decRef();
  }
  util::delete_pointers(
      constant_pool, constant_pool + code->constant_pool.size());
  delete [] constant_pool;
  constant_pool = NULL;
}

void ExecutionContext::resumeExecution(Instruction** pc) {
  assert(pc);
  assert(is_executing);
  assert(constant_pool);

  size_t return_size = local_variables_stack.size();
  // push the current pc as the ret addr
  new_frame(program_counter);
  // set the pc
  program_counter = pc;
  while (true) {
    if (VENOM_UNLIKELY((*program_counter)->execute(*this))) program_counter++;
    // check frame size
    size_t cur_size = local_variables_stack.size();
    assert(cur_size >= return_size);
    if (VENOM_UNLIKELY(cur_size == return_size)) {
      // break execution
      break;
    }
  }
}

void ExecutionContext::resumeExecution(venom_object* obj, size_t index) {
  assert(obj);
  FunctionDescriptor *desc = obj->getClassObj()->vtable.at(index);
  resumeExecution(obj, desc);
}

void ExecutionContext::resumeExecution(venom_object* obj,
                                       FunctionDescriptor* desc) {
  assert(obj);
  assert(desc);
  obj->incRef();
  program_stack.push(venom_cell(obj));
  desc->dispatch(this);
}

Instruction** ExecutionContext::pop_frame() {
  // must decRef() the cells on the stack which need it

  assert(local_variables_stack.size() ==
         local_variables_ref_info_stack.size());

  size_t last_offset = frame_offset.top();
  for (size_t i = last_offset; i < local_variables_stack.size(); i++) {
    if (local_variables_ref_info_stack[i]) {
      local_variables_stack[i].decRef();
    }
  }

  local_variables_stack.resize(last_offset);
  local_variables_ref_info_stack.resize(last_offset);

  Instruction** ret_addr = ret_addr_stack.top();
  ret_addr_stack.pop();
  frame_offset.pop();

  AssertProgramFrameSanity();
  return ret_addr;
}

// TODO: replace this with a thread-local
ExecutionContext* ExecutionContext::_current(NULL);

void FunctionDescriptor::dispatch(ExecutionContext* ctx) {
  assert(ctx);

  if (native) {
    switch (num_args) {

#include <backend/dispatch_cases.inc>

    // TODO: This is quite messy/fragile - we should really replace this with
    // some inline assembly. Also, right now g++'s limit is a 59-argument
    // function, so we can't do any more than w/o inline assembly.

    default: VENOM_UNIMPLEMENTED;
    }
  } else {
    ctx->resumeExecution(
        ctx->code->instructions.begin() + int64_t(function_ptr));
  }
}

}
}
