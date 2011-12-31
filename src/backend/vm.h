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

	runtime::venom_cell& execute() {
		assert(*program_counter);
		while (true) {
			if ((*program_counter)->execute(*this)) program_counter++;
			if (!(*program_counter)) return program_stack.top();
		}
	}

protected:
	Instruction** program_counter;
	Instruction** label_map;

	std::stack<runtime::venom_cell> program_stack;
};

}
}

#endif /* VENOM_BACKEND_VM_H */
