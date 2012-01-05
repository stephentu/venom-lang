#include <cassert>
#include <iostream>

#include <parser/driver.h>
#include <ast/statement/node.h>

using namespace std;
using namespace venom;

int main(int argc, char **argv) {
  string fname;
  for (int ai = 1; ai < argc; ++ai) {
    if (argv[ai] == string ("-p")) {
      global_compile_opts.trace_parse = true;
    } else if (argv[ai] == string ("-s")) {
      global_compile_opts.trace_lex   = true;
    } else {
      fname = argv[ai];
    }
  }

  if (!fname.empty()) {
    compile_result result;
    // TODO: exit code of program
    if (compile_and_exec(fname, result)) {
      return 0;
    } else {
      cerr << result.message << endl;
      return 1;
    }
  }

  // TODO: build a real repl
  string line;
  while (cout << "input: " && getline(cin, line) && !line.empty()) {
    ParseContext pctx;
    Driver driver(pctx);
    bool result = driver.parse_string(line, "input");
    if (result) {
      assert(pctx.stmts != NULL);
      pctx.stmts->print(cout);
      cout << endl;
    }
  }
}
