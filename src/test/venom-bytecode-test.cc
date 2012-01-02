#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <backend/bytecode.h>
#include <backend/vm.h>
#include <runtime/venomobject.h>

class venom_test_check_fail : public std::runtime_error {
public:
  explicit venom_test_check_fail(const std::string& what)
    : std::runtime_error(what) {}
};

#define VENOM_TEST_CHECK(pred) \
  if (!(pred)) throw venom_test_check_fail("Condition `" #pred "' failed")

using namespace std;
using namespace venom::backend;
using namespace venom::runtime;

struct eq_check {
  eq_check(const venom_cell& expect) : expect(expect) {}
  void check(const venom_cell& actual) const {
    VENOM_TEST_CHECK(actual == expect);
  }
  venom_cell expect;
};

template <typename Functor>
void run_test_success_op(const string& name, Instruction** istream, Functor f) {
  ExecutionContext ctx(istream, NULL);
  venom_cell &actual = ctx.execute();
  try {
    f.check(actual);
    cout << "Test " << name << " passed" << endl;
  } catch (venom_test_check_fail& ex) {
    cout << "Test " << name << " failed: " << ex.what() << endl;
  } catch (exception& ex) {
    cout << "Test " << name << " failed with uncaught exception: "
      << ex.what() << endl;
  }
}

void run_test_success(const string& name, Instruction** istream,
                      const venom_cell& expect) {
  run_test_success_op(name, istream, eq_check(expect));
}

struct alloc_obj_check {
  void check(const venom_cell& actual) const {
    VENOM_TEST_CHECK(actual.isObject());
    VENOM_TEST_CHECK(actual.asRawObject());
    VENOM_TEST_CHECK(actual.asRawObject()->cell(0) == venom_cell(100));
    VENOM_TEST_CHECK(actual.asRawObject()->cell(1).isObject());
    VENOM_TEST_CHECK(!actual.asRawObject()->cell(1).asRawObject());
  }
};

int main(int argc, char **argv) {
  {
    Instruction *istream[] = {
      new InstFormatA(Instruction::ALLOC_OBJ, 2),
      new InstFormatA(Instruction::DUP, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 100),
      new InstFormatA(Instruction::SET_ATTR_OBJ, 0),
      new Instruction(Instruction::PUSH_CELL_NIL),
      new InstFormatA(Instruction::SET_ATTR_OBJ, 1),
      NULL
    };
    run_test_success_op("ALLOC_OBJ", istream, alloc_obj_check());
  }

  {
    Instruction *istream[] = {
      new InstFormatC(Instruction::PUSH_CELL_INT, 32),
      new InstFormatC(Instruction::PUSH_CELL_INT, 64),
      new Instruction(Instruction::BINOP_ADD),
      NULL
    };
    run_test_success("BINOP_ADD", istream, venom_cell(32 + 64));
  }
  return 0;
}
