#include <cassert>
#include <iostream>
#include <fstream>

#include <ast/include.h>
#include <analysis/semanticcontext.h>
#include <bootstrap/analysis.h>

#include <driver.h>

using namespace std;
using namespace venom;

int main(int argc, char **argv) {
  ParseContext pctx;
  Driver driver(pctx);
  bool readfile = false;

  for (int ai = 1; ai < argc; ++ai) {
    if (argv[ai] == string ("-p")) {
      driver.trace_parsing = true;
    } else if (argv[ai] == string ("-s")) {
      driver.trace_scanning = true;
    } else {
      // read a file with expressions

      fstream infile(argv[ai]);
      if (!infile.good()) {
        cerr << "Could not open file: " << argv[ai] << endl;
        return 0;
      }

      bool result = driver.parse_stream(infile, argv[ai]);
      if (result) {
        assert(pctx.stmts != NULL);
        pctx.stmts->print(cout);
        cout << endl;
      }
      readfile = true;
    }
  }

  if (readfile) {
    if (pctx.stmts) {
      try {
        analysis::SemanticContext ctx("main");

        // bootstrap
        analysis::SymbolTable *root = bootstrap::NewBootstrapSymbolTable(&ctx);
        pctx.stmts->initSymbolTable(root->newChildScope(pctx.stmts));

        // semantic check
        pctx.stmts->semanticCheck(&ctx);

        // type check
        pctx.stmts->typeCheck(&ctx);

      } catch (analysis::SemanticViolationException& e) {
        cerr << "Semantic Violation: " << e.what() << endl;
      } catch (analysis::TypeViolationException& e) {
        cerr << "Typecheck Violation: " << e.what() << endl;
      }
    }
    return 0;
  }

  string line;
  while (cout << "input: " && getline(cin, line) && !line.empty()) {
    bool result = driver.parse_string(line, "input");
    if (result) {
      assert(pctx.stmts != NULL);
      pctx.stmts->print(cout);
      cout << endl;
    }
  }
}
