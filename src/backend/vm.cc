#include <backend/vm.h>
#include <util/scopehelpers.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

void ExecutionContext::execute(Callback& callback) {
  assert(*program_counter);
  util::ScopedBoolean sb(is_executing);
  util::ScopedVariable<ExecutionContext*> sv(_current, this);
  new_frame();
  while (true) {
    if ((*program_counter)->execute(*this)) program_counter++;
    if (!(*program_counter)) {
      // end of stream
      venom_cell ret = program_stack.top();
      program_stack.pop();
      pop_frame();
      callback.handleResult(ret);
      return;
    }
  }
}

void ExecutionContext::resumeExecution(Instruction** pc) {
  assert(pc);
  assert(is_executing);
  // push the current pc as the ret addr
  program_stack.push(venom_cell(int64_t(program_counter)));
  // new frame
  size_t return_size = local_variables_stack.size();
  new_frame();
  // set the pc
  program_counter = pc;
  while (true) {
    if ((*program_counter)->execute(*this)) program_counter++;
    // check frame size
    size_t cur_size = local_variables_stack.size();
    assert(cur_size >= return_size);
    if (cur_size == return_size) {
      // break execution
      break;
    }
  }
}

void ExecutionContext::resumeExecution(venom_object* obj, size_t index) {
  FunctionDescriptor *desc = obj->getClassObj()->vtable.at(index);
  assert(desc);
  program_stack.push(venom_cell(obj));
  desc->dispatch(this);
}

// TODO: replace this with a thread-local
ExecutionContext* ExecutionContext::_current(NULL);

void FunctionDescriptor::dispatch(ExecutionContext* ctx) {
  assert(ctx);
  if (native) {
    switch (num_args) {
    case 0: {
      F0 f = reinterpret_cast<F0>(function_ptr);
      venom_object_ptr ret = f(ctx);
      ctx->program_stack.push(venom_cell(ret.get()));
      break;
    }
    case 1: {
      F1 f = reinterpret_cast<F1>(function_ptr);
      venom_cell arg0 = ctx->program_stack.top(); ctx->program_stack.pop();
      venom_object_ptr ret = f(ctx, arg0.box());
      ctx->program_stack.push(venom_cell(ret.get()));
      break;
    }
    // TODO: more arguments...
    default: VENOM_UNIMPLEMENTED;
    }
  } else {
    ctx->resumeExecution(reinterpret_cast<Instruction**>(function_ptr));
  }
}

}
}
