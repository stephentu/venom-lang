/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
      global_compile_opts.trace_lex = true;
    } else if (argv[ai] == string ("-c")) {
      global_compile_opts.semantic_check_only = true;
    } else if (argv[ai] == string ("--print-ast")) {
      global_compile_opts.print_ast = true;
    } else if (argv[ai] == string ("--print-bytecode")) {
      global_compile_opts.print_bytecode = true;
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
