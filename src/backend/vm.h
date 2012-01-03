#ifndef VENOM_BACKEND_VM_H
#define VENOM_BACKEND_VM_H

#include <cassert>
#include <stack>

#include <backend/bytecode.h>
#include <runtime/venomobject.h>

namespace venom {
namespace backend {

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
  };

  /** Does *not* take ownership of program_counter or constant_pool */
  ExecutionContext(Instruction** program_counter,
                   runtime::venom_cell** constant_pool,
                   runtime::venom_class_object** class_obj_pool)
    : program_counter(program_counter), constant_pool(constant_pool),
      class_obj_pool(class_obj_pool), is_executing(false) {}

  void execute(Callback& callback);

  /** Returns the currently executing context in the current thread */
  inline static ExecutionContext* current_context() { return _current; }
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

  inline void new_frame() {
    local_variables_stack.push(std::vector<runtime::venom_cell>());
  }
  inline void pop_frame() { local_variables_stack.pop(); }

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
  typedef runtime::venom_object_ptr vptr;
  typedef ExecutionContext* exptr;

  // TODO: more typedefs
  typedef vptr(*F0)(exptr);
  typedef vptr(*F1)(exptr,vptr);
  typedef vptr(*F2)(exptr,vptr,vptr);
  typedef vptr(*F3)(exptr,vptr,vptr,vptr);
  typedef vptr(*F4)(exptr,vptr,vptr,vptr,vptr);
  typedef vptr(*F5)(exptr,vptr,vptr,vptr,vptr,vptr);
  typedef vptr(*F6)(exptr,vptr,vptr,vptr,vptr,vptr,vptr);
  typedef vptr(*F7)(exptr,vptr,vptr,vptr,vptr,vptr,vptr,vptr);
  typedef vptr(*F8)(exptr,vptr,vptr,vptr,vptr,vptr,vptr,vptr,vptr);

  FunctionDescriptor(void* function_ptr, size_t num_args, bool native)
    : function_ptr(function_ptr), num_args(num_args), native(native) {}

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
  bool native;
};

}
}

#endif /* VENOM_BACKEND_VM_H */
