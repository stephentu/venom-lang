#include <backend/vm.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

venom_cell ExecutionContext::execute() {
  assert(*program_counter);
  new_frame();
  while (true) {
    if ((*program_counter)->execute(*this)) program_counter++;
    if (!(*program_counter)) {
      // end of stream
      venom_cell ret = program_stack.top();
      program_stack.pop();
      pop_frame();
      return ret;
    }
  }
}

}
}
