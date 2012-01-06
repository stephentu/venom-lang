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
      program_counter(code->instructions.begin()),
      constant_pool(NULL),
      class_obj_pool(code->class_obj_pool.begin()),
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

  inline std::vector<runtime::venom_cell>& local_variables() {
    return local_variables_stack.top();
  }
  inline const std::vector<runtime::venom_cell>& local_variables() const {
    return local_variables_stack.top();
  }

  inline std::vector<bool>& local_variables_ref_info() {
    return local_variables_ref_info_stack.top();
  }
  inline const std::vector<bool>& local_variables_ref_info() const {
    return local_variables_ref_info_stack.top();
  }

  inline void new_frame() {
    local_variables_stack.push(std::vector<runtime::venom_cell>());
    local_variables_ref_info_stack.push(std::vector<bool>());
  }

  void pop_frame();

  /** Linked program */
  Executable* code;

  /** Complete linked program instruction stream */
  Instruction** program_counter;

  /** Initialized constant pool */
  runtime::venom_cell** constant_pool;

  /** Initialized class object pool */
  runtime::venom_class_object** class_obj_pool;

  /** Program stack - shared per context */
  std::stack<runtime::venom_cell> program_stack;

  /** Program frames - created per function invocation */
  std::stack< std::vector<runtime::venom_cell> > local_variables_stack;

  std::stack< std::vector<bool> > local_variables_ref_info_stack;

  /** Is this context currently executing? */
  bool is_executing;

  /**
   * The currently executing context in this thread.
   * TODO: make thread local
   */
  static ExecutionContext* _current;
};

/**
 * TODO: document FunctionDescriptor
 */
class FunctionDescriptor {
  friend class ExecutionContext;
  friend class Instruction;
public:
  typedef runtime::venom_cell vcell;
  typedef runtime::venom_ret_cell vrcell;
  typedef ExecutionContext* exptr;

  // TODO: more typedefs
  typedef vrcell(*F0)(exptr);
  typedef vrcell(*F1)(exptr,vcell);
  typedef vrcell(*F2)(exptr,vcell,vcell);
  typedef vrcell(*F3)(exptr,vcell,vcell,vcell);
  typedef vrcell(*F4)(exptr,vcell,vcell,vcell,vcell);
  typedef vrcell(*F5)(exptr,vcell,vcell,vcell,vcell,vcell);
  typedef vrcell(*F6)(exptr,vcell,vcell,vcell,vcell,vcell,vcell);
  typedef vrcell(*F7)(exptr,vcell,vcell,vcell,vcell,vcell,vcell,vcell);
  typedef vrcell(*F8)(exptr,vcell,vcell,vcell,vcell,vcell,vcell,vcell,vcell);

  FunctionDescriptor(void* function_ptr, size_t num_args,
                     uint64_t arg_ref_cell_bitmap, bool native)
    : function_ptr(function_ptr), num_args(num_args),
      arg_ref_cell_bitmap(arg_ref_cell_bitmap), native(native) {
    // TODO: implementation limitation
    assert(num_args <= 64);
  }

  /** Accessors */
  inline void* getFunctionPtr() const { return function_ptr; }
  inline bool getNumArgs() const { return num_args; }
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
