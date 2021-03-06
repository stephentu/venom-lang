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
 * 3. Neither the name of the author nor the names
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

#ifndef VENOM_BACKEND_VM_H
#define VENOM_BACKEND_VM_H

#include <cassert>
#include <stack>
#include <stdexcept>
#include <vector>

#include <backend/bytecode.h>
#include <backend/linker.h>

#include <runtime/venomobject.h>

#include <util/container.h>
#include <util/stl.h>

namespace venom {
namespace backend {

class VenomRuntimeException : public std::runtime_error {
public:
  explicit VenomRuntimeException(const std::string& msg)
    : std::runtime_error(msg) {}
};

/**
 * ExecutionContext represents a thread of execution in the venom virtual
 * machine.
 */
class ExecutionContext {
  friend class FunctionDescriptor;
  friend class Instruction;
  friend class runtime::venom_object;
public:

  typedef std::stack<runtime::venom_cell> program_stack_type;

  /**
   * Callback class to read a result from execute()
   *
   * We do not use functors with templates for execute(), because we cannot put
   * execute() in the header file (w/o some un-necessary complications)
   */
  class Callback {
  public:
    virtual ~Callback() {}
    virtual void handleResult(runtime::venom_cell& result) = 0;
    virtual void noResult() = 0;
  };

  class DefaultCallback : public Callback {
  public:
    virtual void handleResult(runtime::venom_cell& result) {};
    virtual void noResult() {}
  };

  /** Does *not* take ownership of executable */
  ExecutionContext(Executable* code)
    : code(code),
      program_counter(code->startingInst()),
      constant_pool(NULL),
      is_executing(false) {}

  ~ExecutionContext() { assert(!constant_pool); }

  void execute(Callback& callback);

  /** Returns the currently executing context in the current thread */
  inline static ExecutionContext* current_context() { return _current; }

private:
  struct scoped_constants {
    scoped_constants(ExecutionContext* ctx)
      : ctx(ctx) { ctx->initConstants(); }
    ~scoped_constants() { ctx->releaseConstants(); }
    ExecutionContext* ctx;
  };

  void initConstants();
  void releaseConstants();

protected:

  /**
   * Can only be called while this context is executing.  This sets the
   * program_counter to pc and starts executing in a new frame until the new
   * frame is popped. At that point resumeExecution() returns control to the
   * caller, restoring the program_counter to what it was before
   * resumeExecution() was called. However, whatever modifications made to the
   * program stack are kept.
   *
   * This method is most useful for native functions to call back into the
   * interpreter via a function.  The typical use case is to set up the
   * arguments to a function on the stack, and then call resumeExecution() with
   * the pc which points to the beginnning of the function. After
   * resumeExecution() returns, the arguments passed to the function are popped
   * off the stack and replaced with the return value of the function (at the
   * top of the stack). The pc appears to be un-modified from the point of view
   * of the caller of resumeExecution().
   */
  void resumeExecution(Instruction** pc);

  /**
   * Do virtual method dispatch on the index-th entry of obj's
   * vtable. Has the same semantics as resumeExecution() does (on an instruction),
   * expect this resumeExecution() possibly calls into native code.
   * resumeExecution() does *NOT* expect the "this" pointer to be set up
   * on the stack (but the rest of the arguments should be).
   */
  void resumeExecution(runtime::venom_object* obj, size_t index);

  void resumeExecution(runtime::venom_object* obj, FunctionDescriptor* desc);

  inline runtime::venom_cell& local_variable(size_t n) {
    return local_variables_stack[frame_offset.top() + n];
  }
  inline const runtime::venom_cell& local_variable(size_t n) const {
    return local_variables_stack[frame_offset.top() + n];
  }

  inline bool local_variable_ref_info(size_t n) const {
    return local_variables_ref_info_stack[frame_offset.top() + n];
  }

  inline void new_frame(Instruction** ret_addr) {
    AssertProgramFrameSanity();

    local_variables_stack.reserve(local_variables_stack.size() + 16);
    local_variables_ref_info_stack.reserve(local_variables_stack.size() + 16);

    frame_offset.push(local_variables_stack.size());
    ret_addr_stack.push(ret_addr);
  }

  Instruction** pop_frame();

  /** Linked program */
  Executable* code;

  /** Complete linked program instruction stream */
  Instruction** program_counter;

  /** Initialized constant pool */
  runtime::venom_cell** constant_pool;

  /** Program stack - shared per context */
  program_stack_type program_stack;

  /** Program frames - created per function invocation */
  std::vector< runtime::venom_cell > local_variables_stack;

  std::vector< bool > local_variables_ref_info_stack;

  std::stack< size_t > frame_offset;

  std::stack< Instruction** > ret_addr_stack;

  /** Is this context currently executing? */
  bool is_executing;

  /**
   * The currently executing context in this thread.
   * TODO: make thread local
   */
  static ExecutionContext* _current;

private:
  inline void AssertProgramFrameSanity() const {
    assert(local_variables_stack.size() ==
           local_variables_ref_info_stack.size());

    assert(frame_offset.size() == ret_addr_stack.size());
  }
  inline void primeStacks() {
    // TODO: tune these constants
    local_variables_stack.reserve(2048);
    local_variables_ref_info_stack.reserve(2048);
  }
};

/**
 * TODO: document FunctionDescriptor
 */
class FunctionDescriptor {
  friend class ExecutionContext;
  friend class Instruction;
public:
  typedef runtime::venom_cell vc;
  typedef runtime::venom_ret_cell vr;
  typedef ExecutionContext* ec;

  /** 64 argument functions is the maximum that is supported
   * by the system */
  // TODO: implementation limitation
  static const uint32_t MaxNumArgs = 64;

  typedef vr(*F0) (ec);
  typedef vr(*F1) (ec,vc);
  typedef vr(*F2) (ec,vc,vc);
  typedef vr(*F3) (ec,vc,vc,vc);
  typedef vr(*F4) (ec,vc,vc,vc,vc);
  typedef vr(*F5) (ec,vc,vc,vc,vc,vc);
  typedef vr(*F6) (ec,vc,vc,vc,vc,vc,vc);
  typedef vr(*F7) (ec,vc,vc,vc,vc,vc,vc,vc);
  typedef vr(*F8) (ec,vc,vc,vc,vc,vc,vc,vc,vc);
  typedef vr(*F9) (ec,vc,vc,vc,vc,vc,vc,vc,vc,vc);

#define _10_VCS vc,vc,vc,vc,vc,vc,vc,vc,vc,vc
#define _20_VCS _10_VCS,_10_VCS
#define _30_VCS _20_VCS,_10_VCS
#define _40_VCS _30_VCS,_10_VCS
#define _50_VCS _40_VCS,_10_VCS
#define _60_VCS _50_VCS,_10_VCS

#define _IMPL_FCN_TYPEDEFS(prefix) \
  typedef vr(*F ## prefix ## 0)(ec,_ ## prefix ## 0_VCS); \
  typedef vr(*F ## prefix ## 1)(ec,_ ## prefix ## 0_VCS,vc); \
  typedef vr(*F ## prefix ## 2)(ec,_ ## prefix ## 0_VCS,vc,vc); \
  typedef vr(*F ## prefix ## 3)(ec,_ ## prefix ## 0_VCS,vc,vc,vc); \
  typedef vr(*F ## prefix ## 4)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc); \
  typedef vr(*F ## prefix ## 5)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc,vc); \
  typedef vr(*F ## prefix ## 6)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc,vc,vc); \
  typedef vr(*F ## prefix ## 7)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc,vc,vc,vc); \
  typedef vr(*F ## prefix ## 8)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc,vc,vc,vc,vc); \
  typedef vr(*F ## prefix ## 9)(ec,_ ## prefix ## 0_VCS,vc,vc,vc,vc,vc,vc,vc,vc,vc);

  _IMPL_FCN_TYPEDEFS(1);
  _IMPL_FCN_TYPEDEFS(2);
  _IMPL_FCN_TYPEDEFS(3);
  _IMPL_FCN_TYPEDEFS(4);
  _IMPL_FCN_TYPEDEFS(5);

  typedef vr(*F60) (ec _60_VCS);
  typedef vr(*F61) (ec,_60_VCS,vc);
  typedef vr(*F62) (ec,_60_VCS,vc,vc);
  typedef vr(*F63) (ec,_60_VCS,vc,vc,vc);
  typedef vr(*F64) (ec,_60_VCS,vc,vc,vc,vc);

#undef _IMPL_FCN_TYPEDEFS

#undef _10_VCS
#undef _20_VCS
#undef _30_VCS
#undef _40_VCS
#undef _50_VCS
#undef _60_VCS

  FunctionDescriptor(void* function_ptr, size_t num_args,
                     uint64_t arg_ref_cell_bitmap, bool native)
    : function_ptr(function_ptr), num_args(num_args),
      arg_ref_cell_bitmap(arg_ref_cell_bitmap), native(native) {
    assert(num_args <= MaxNumArgs);
  }

  /** Accessors */
  inline void* getFunctionPtr() const { return function_ptr; }
  inline size_t getNumArgs() const { return num_args; }
  inline uint64_t argRefCellBitmap() const { return arg_ref_cell_bitmap; }
  inline bool isNative() const { return native; }

protected:
  /** Assumes stack is properly set up before invocation; has the same
   * semantics as ExecutionContext::resumeExecution() */
  void dispatch(ExecutionContext* ctx);

private:
  void* function_ptr;
  size_t num_args;
  uint64_t arg_ref_cell_bitmap;
  bool native;
};

typedef std::vector<FunctionDescriptor*> FuncDescVec;

}
}

#endif /* VENOM_BACKEND_VM_H */
