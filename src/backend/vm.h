#ifndef VENOM_BACKEND_VM_H
#define VENOM_BACKEND_VM_H

#include <cassert>
#include <stack>

#include <backend/bytecode.h>
#include <runtime/venomobject.h>

namespace venom {
namespace backend {

class ExecutionContext {
  friend class Instruction;
public:
  /** Does *not* take ownership of program_counter or label_map */
  ExecutionContext(Instruction** program_counter,
                   Instruction** label_map)
    : program_counter(program_counter), label_map(label_map) {
    assert(program_counter);
  }

  runtime::venom_cell execute();

protected:
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

  Instruction** program_counter;
  Instruction** label_map;

  std::stack<runtime::venom_cell> program_stack;
  std::stack< std::vector<runtime::venom_cell> > local_variables_stack;
};

}
}

#endif /* VENOM_BACKEND_VM_H */
