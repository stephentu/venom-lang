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

  new_frame(NULL); // denotes when <main> returns
  while (true) {
    if (VENOM_UNLIKELY((*program_counter)->execute(*this))) program_counter++;
    if (VENOM_UNLIKELY(program_counter == NULL)) {
      // end of stream
      assert(local_variables_stack.empty());
      assert(local_variables_ref_info_stack.empty());
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
  vector<venom_cell>& locVars = local_variables();
  vector<bool>& refInfo = local_variables_ref_info();

  assert(locVars.size() == refInfo.size());

  for (size_t i = 0; i < locVars.size(); i++) {
    if (refInfo[i]) locVars[i].decRef();
  }

  local_variables_stack.pop();
  local_variables_ref_info_stack.pop();
  Instruction** ret_addr = ret_addr_stack.top();
  ret_addr_stack.pop();

  AssertProgramFrameSanity();
  return ret_addr;
}

// TODO: replace this with a thread-local
ExecutionContext* ExecutionContext::_current(NULL);

void FunctionDescriptor::dispatch(ExecutionContext* ctx) {
  assert(ctx);
  if (native) {
    switch (num_args) {
    case 0: {
      F0 f = reinterpret_cast<F0>(function_ptr);
      venom_ret_cell ret = f(ctx);
      ctx->program_stack.push(ret);
      break;
    }
    case 1: {
      F1 f = reinterpret_cast<F1>(function_ptr);
      venom_cell arg0 = ctx->program_stack.top(); ctx->program_stack.pop();
      venom_ret_cell ret = f(ctx, arg0);
      ctx->program_stack.push(ret);
      if (arg_ref_cell_bitmap & 0x1) arg0.decRef();
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
