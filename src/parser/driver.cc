/** driver.cc Implementation of the venom::Driver class. */

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <parser/driver.h>
#include <parser/scanner.h>

#include <analysis/semanticcontext.h>
#include <ast/include.h>
#include <bootstrap/analysis.h>

using namespace std;

namespace venom {

Driver::Driver(ParseContext& ctx)
    : trace_scanning(false),
      trace_parsing(false),
      ctx(ctx) {}

bool Driver::parse_stream(istream& in, const string& sname)
{
    streamname = sname;

    Scanner scanner(&in);
    scanner.set_debug(trace_scanning);
    this->lexer = &scanner;

    Parser parser(*this);
    parser.set_debug_level(trace_parsing);
    return (parser.parse() == 0);
}

bool Driver::parse_file(const string &filename)
{
    ifstream in(filename.c_str());
    if (!in.good()) return false;
    return parse_stream(in, filename);
}

bool Driver::parse_string(const string &input, const string& sname)
{
    istringstream iss(input);
    return parse_stream(iss, sname);
}

void Driver::error(const class location& l, const string& m)
{
    cerr << l << ": " << m << endl;
}

void Driver::error(const string& m)
{
    cerr << m << endl;
}

bool compile(const string& fname,
             const compile_opts& opts,
             compile_result& result) {
  fstream infile(fname.c_str());
  if (!infile.good()) {
    throw invalid_argument("Invalid filename: " + fname);
  }
  ParseContext pctx;
  Driver driver(pctx);
  if (opts.trace_lex) driver.trace_scanning = true;
  if (opts.trace_parse) driver.trace_parsing = true;
  bool validSyntax = driver.parse_stream(infile, fname);
  if (!validSyntax) {
    result.result = compile_result::InvalidSyntax;
    result.message = "Invalid syntax";
    return false;
  }
  assert(pctx.stmts != NULL);
  if (opts.print_ast) {
    pctx.stmts->print(cerr);
    cerr << endl;
  }
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
    result.result  = compile_result::SemanticError;
    result.message = string("Semantic Violation: ") + e.what();
    return false;
  } catch (analysis::TypeViolationException& e) {
    result.result  = compile_result::TypeError;
    result.message = string("Typecheck Violation: ") + e.what();
    return false;
  } catch (exception& e) {
    result.result  = compile_result::UnknownError;
    result.message = string("Uncaught Exception: ") + e.what();
    return false;
  }
  result.result  = compile_result::Success;
  result.message = "";
  return true;
}


} // namespace venom
