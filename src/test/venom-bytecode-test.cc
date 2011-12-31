#include <iostream>
#include <string>
#include <vector>

#include <backend/bytecode.h>
#include <backend/vm.h>
#include <runtime/venomobject.h>

using namespace std;
using namespace venom::backend;
using namespace venom::runtime;

void run_test_success(const string& name, Instruction** istream,
                      const venom_cell& expect) {
  ExecutionContext ctx(istream, NULL);
  venom_cell &actual = ctx.execute();
  if (actual != expect) {
    cout << "Test " << name << " failed: " <<
      "Expected " << expect << ", got " << actual << endl;
  } else {
    cout << "Test " << name << " passed" << endl;
  }
}

int main(int argc, char **argv) {
  {
    Instruction *istream[] = {
      new InstFormatC(Instruction::PUSH_CELL, venom_cell(32)),
      new InstFormatC(Instruction::PUSH_CELL, venom_cell(64)),
      new Instruction(Instruction::BINOP_ADD),
      NULL
    };
    run_test_success("BINOP_ADD", istream, venom_cell(32 + 64));
  }
  return 0;
}
