#include <iostream>
#include <sstream>
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
#define VENOM_TEST_CHECK_EQ(a, b) \
  do { \
    if (!((a) == (b))) { \
      std::stringstream buf; \
      buf << "Condition `" << (a) << " == " << (b) << "' failed"; \
      throw venom_test_check_fail(buf.str()); \
    } \
  } while (0)

using namespace std;
using namespace venom::backend;
using namespace venom::runtime;

struct eq_check {
  eq_check(const venom_cell& expect) : expect(expect) {}
  void check(const venom_cell& actual) const {
    VENOM_TEST_CHECK_EQ(actual, expect);
  }
  venom_cell expect;
};

template <typename Functor>
void run_test_success_op(const string& name, Instruction** istream, Functor f) {
  ExecutionContext ctx(istream, NULL);
  venom_cell actual = ctx.execute();
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
      new InstFormatU32(Instruction::ALLOC_OBJ, 2),
      new InstFormatU32(Instruction::DUP, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 100),
      new InstFormatU32(Instruction::SET_ATTR_OBJ, 0),
      new Instruction(Instruction::PUSH_CELL_NIL),
      new InstFormatU32(Instruction::SET_ATTR_OBJ, 1),
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

  {

    Instruction *istream[] = {
      // C code:
      //
      // int n = 0;
      // int m = 1;
      // int i = 2;
      // for (; i <= 5; i++) {
      //   int tmp = m + n;
      //   n = m;
      //   m = tmp;
      // }
      //
      // Bytecode:
      //
      // PUSH_CELL_INT 0
      // STORE_LOCAL_VAR 0 ; var[0] = n
      // PUSH_CELL_INT 1
      // STORE_LOCAL_VAR 1 ; var[1] = m
      // PUSH_CELL_INT 2
      // STORE_LOCAL_VAR 2 ; var[2] = i
      //
      // loop:
      // LOAD_LOCAL_VAR 2
      // PUSH_CELL_INT 5
      // BINOP_CMP_LE
      // BRANCH_Z done ; if (i > 5) goto done
      // LOAD_LOCAL_VAR 0
      // LOAD_LOCAL_VAR 1
      // BINOP_ADD
      // LOAD_LOCAL_VAR 1
      // STORE_LOCAL_VAR 0
      // STORE_LOCAL_VAR 1
      // LOAD_LOCAL_VAR 2
      // PUSH_CELL_INT 1
      // BINOP_ADD
      // STORE_LOCAL_VAR 2
      // JUMP loop
      //
      // done:
      // LOAD_LOCAL_VAR 1

      new InstFormatC(Instruction::PUSH_CELL_INT, 0),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 0),
      new InstFormatC(Instruction::PUSH_CELL_INT, 1),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 1),
      new InstFormatC(Instruction::PUSH_CELL_INT, 2),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 2),

      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 5),
      new Instruction(Instruction::BINOP_CMP_LE),
      new InstFormatI32(Instruction::BRANCH_Z, 11 /* done */),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 0),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
      new Instruction(Instruction::BINOP_ADD),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 0),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 1),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 1),
      new Instruction(Instruction::BINOP_ADD),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 2),
      new InstFormatI32(Instruction::JUMP, -15 /* loop */),

      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
      NULL
    };
    run_test_success("fibn", istream, venom_cell(5 /* fib(5) */));
  }

  return 0;
}
