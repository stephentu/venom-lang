#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <backend/bytecode.h>
#include <backend/linker.h>
#include <backend/vm.h>

#include <runtime/builtin.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

#include <util/container.h>
#include <util/stl.h>

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

#define VENOM_TEST_FAIL VENOM_TEST_CHECK(false)

using namespace std;
using namespace venom;
using namespace venom::backend;
using namespace venom::runtime;

struct base_check {
  void beforeTest() {}
  void check(venom_cell& actual) {}
  void checkNoResult(venom_cell& actual) {}
  void afterTest() {}
};

#define EQ_CHECK_IMPL(lowername, uppername, type) \
  struct eq_check_##lowername : public base_check { \
    eq_check_##lowername(type expect) : expect(expect) {} \
    void check(venom_cell& actual) { \
      VENOM_TEST_CHECK_EQ(actual.as ## uppername (), expect); \
    } \
    void checkNoResult() const { VENOM_TEST_FAIL; } \
    type expect; \
  };

EQ_CHECK_IMPL(int, Int, int64_t)
EQ_CHECK_IMPL(float, Double, double)
EQ_CHECK_IMPL(bool, Bool, bool)

static venom_class_object* obj_pool[] = {
  &venom_object::ObjClassTable,
  &venom_string::StringClassTable,
  new venom_class_object("test_obj", sizeof(venom_object), 2, 0x2,
                         venom_object::ObjClassTable.vtable),
};

template <typename Functor>
class TestCallback : public ExecutionContext::Callback {
public:
  TestCallback(const string& name, Functor functor)
    : name(name), functor(functor) {}
  virtual void handleResult(venom_cell& result) {
    functor.afterTest();
    try {
      functor.check(result);
      cout << "Test " << name << " passed" << endl;
    } catch (venom_test_check_fail& ex) {
      cout << "Test " << name << " failed: " << ex.what() << endl;
    } catch (exception& ex) {
      cout << "Test " << name << " failed with uncaught exception: "
        << ex.what() << endl;
    }
  }
  virtual void noResult() {
    functor.afterTest();
    try {
      functor.checkNoResult();
      cout << "Test " << name << " passed" << endl;
    } catch (venom_test_check_fail& ex) {
      cout << "Test " << name << " failed: " << ex.what() << endl;
    } catch (exception& ex) {
      cout << "Test " << name << " failed with uncaught exception: "
        << ex.what() << endl;
    }
  }
private:
  const string name;
  Functor functor;
};

typedef Executable::ConstPool ConstPool;
typedef Executable::IStream IStream;
typedef Executable::ClassObjPool ClassObjPool;

template <typename Functor>
void run_test_success_op(const string& name, const IStream& istream, Functor f) {
  TestCallback<Functor> callback(name, f);
  Executable exec(
      ConstPool(), ClassObjPool(obj_pool, VENOM_NELEMS(obj_pool)), istream);
  ExecutionContext ctx(&exec);
  f.beforeTest();
  ctx.execute(callback);
}

void run_test_success(const string& name, const IStream& istream,
                      int64_t expect) {
  run_test_success_op(name, istream, eq_check_int(expect));
}

void run_test_success(const string& name, const IStream& istream,
                      double expect) {
  run_test_success_op(name, istream, eq_check_float(expect));
}

void run_test_success(const string& name, const IStream& istream,
                      bool expect) {
  run_test_success_op(name, istream, eq_check_bool(expect));
}

struct alloc_obj_check : public base_check {
  void check(venom_cell& actual) const {
    VENOM_TEST_CHECK(actual.asRawObject());
    VENOM_TEST_CHECK_EQ(actual.asRawObject()->getCount(), 1);
    VENOM_TEST_CHECK_EQ(actual.asRawObject()->cell(0).asInt(), 100);
    VENOM_TEST_CHECK(!actual.asRawObject()->cell(1).asRawObject());
    actual.decRef();
  }
  void checkNoResult() const { VENOM_TEST_FAIL; }
};

struct print_check : public base_check {
  print_check(stringstream* sb) : sb(sb) {}
  void beforeTest() {
    save = venom_stdout.rdbuf();
    venom_stdout.rdbuf(sb->rdbuf());
  }
  void check(venom_cell& actual) const {
    VENOM_TEST_CHECK_EQ(sb->str(), "1337\n");
  }
  void checkNoResult() const { VENOM_TEST_FAIL; }
  void afterTest() { venom_stdout.rdbuf(save); }
  stringstream* sb;
  streambuf* save;
};

static IStream makeIStream(Instruction **istream, size_t n_elems) {
  Instruction **i = new Instruction* [n_elems];
  memcpy(i, istream, n_elems * sizeof(Instruction*));
  return IStream(i, n_elems);
}

int main(int argc, char **argv) {
  {
    Instruction *istream[] = {
      new InstFormatU32(Instruction::ALLOC_OBJ, 2 /* test_obj */),
      new InstFormatU32(Instruction::DUP_REF, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 100),
      new InstFormatU32(Instruction::SET_ATTR_OBJ, 0),
      new Instruction(Instruction::PUSH_CELL_NIL),
      new InstFormatU32(Instruction::SET_ATTR_OBJ_REF, 1),
    };
    run_test_success_op("ALLOC_OBJ",
                        makeIStream(istream, VENOM_NELEMS(istream)),
                        alloc_obj_check());
  }

  {
    Instruction *istream[] = {
      new InstFormatC(Instruction::PUSH_CELL_INT, 32),
      new InstFormatC(Instruction::PUSH_CELL_INT, 64),
      new Instruction(Instruction::BINOP_ADD_INT),
    };
    run_test_success("BINOP_ADD_INT",
                     makeIStream(istream, VENOM_NELEMS(istream)),
                     int64_t(32 + 64));
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
      new Instruction(Instruction::BINOP_CMP_LE_INT),
      new InstFormatI32(Instruction::BRANCH_Z_BOOL, 11 /* done */),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 0),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
      new Instruction(Instruction::BINOP_ADD_INT),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 0),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 1),
      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 2),
      new InstFormatC(Instruction::PUSH_CELL_INT, 1),
      new Instruction(Instruction::BINOP_ADD_INT),
      new InstFormatU32(Instruction::STORE_LOCAL_VAR, 2),
      new InstFormatI32(Instruction::JUMP, -15 /* loop */),

      new InstFormatU32(Instruction::LOAD_LOCAL_VAR, 1),
    };
    run_test_success("fibn",
                     makeIStream(istream, VENOM_NELEMS(istream)),
                     int64_t(5 /* fib(5) */));
  }

  // TODO: this is currently broken
  //{
  //  Instruction *istream[] = {
  //    new InstFormatC(Instruction::PUSH_CELL_INT, 1337),
  //    new InstFormatIPtr(Instruction::CALL_NATIVE,
  //                       intptr_t(BuiltinPrintDescriptor)),
  //  };
  //  stringstream sb;
  //  run_test_success_op("print",
  //                      makeIStream(istream, VENOM_NELEMS(istream)),
  //                      print_check(&sb));
  //}

  return 0;
}
