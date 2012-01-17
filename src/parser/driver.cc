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

#include <backend/codegenerator.h>
#include <backend/vm.h>

#include <bootstrap/analysis.h>

#include <util/filesystem.h>

using namespace std;

using namespace venom::analysis;
using namespace venom::ast;
using namespace venom::backend;
using namespace venom::bootstrap;

namespace venom {

compile_opts global_compile_opts;

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

void
unsafe_compile_module(const string& fname, fstream& infile,
                      SemanticContext& ctx) {
  ParseContext pctx;
  Driver driver(pctx);
  if (global_compile_opts.trace_lex)   driver.trace_scanning = true;
  if (global_compile_opts.trace_parse) driver.trace_parsing = true;
  bool validSyntax = driver.parse_stream(infile, fname);
  if (!validSyntax) {
    // TODO better error message
    throw ParseErrorException("Invalid syntax");
  }
  assert(pctx.stmts);
  ctx.setModuleRoot(pctx.stmts);
  if (global_compile_opts.print_ast) {
    cerr << "After parse stage:" << endl;
    pctx.stmts->print(cerr);
    cerr << endl;
  }
  pctx.stmts->initSymbolTable(ctx.getRootSymbolTable()->newChildScope(NULL));
  pctx.stmts->semanticCheck(&ctx);
  pctx.stmts->typeCheck(&ctx);
  if (global_compile_opts.print_ast) {
    cerr << "After type check stage:" << endl;
    pctx.stmts->print(cerr);
    cerr << endl;
  }
}

#define _REWRITE_LOCAL_STAGES(x) \
  x(CanonicalRefs) \
  x(ModuleMain) \
  x(FunctionReturns) \
  x(BoxPrimitives)

#define _IMPL_REWRITE_FUNCTOR(stage) \
  struct _rewrite_functor_##stage { \
    inline void operator()(ASTStatementNode* root, \
                           SemanticContext* ctx) const { \
      root->rewriteLocal(ctx, ASTNode::stage); \
      if (global_compile_opts.print_ast) { \
        cerr << "After rewrite local stage (" << #stage << "):" << endl; \
        root->print(cerr); \
        cerr << endl; \
      } \
    } \
  };

_REWRITE_LOCAL_STAGES(_IMPL_REWRITE_FUNCTOR)

#undef _IMPL_REWRITE_FUNCTOR

struct _codegen_functor {
  inline void operator()(ASTStatementNode* root,
                         SemanticContext* ctx) const {
    CodeGenerator cg(ctx);
    root->codeGen(cg);
    if (global_compile_opts.print_bytecode) cg.printDebugStream();
    cg.createObjectCodeAndSet(ctx);
  }
};

void
unsafe_compile(const string& fname, fstream& infile,
               SemanticContext& ctx) {
  assert(!ctx.isRootContext());

  // recursively compile all the modules referenced by
  // infile
  unsafe_compile_module(fname, infile, ctx);

#define _IMPL_REWRITE_LOCAL(stage) \
  do { \
    ctx.getProgramRoot()->forEachModule(_rewrite_functor_##stage()); \
  } while (0);

  _REWRITE_LOCAL_STAGES(_IMPL_REWRITE_LOCAL)

#undef _IMPL_REWRITE_LOCAL

  if (global_compile_opts.semantic_check_only) return;

  ctx.getProgramRoot()->forEachModule(_codegen_functor());
}

static void exec(SemanticContext& ctx) {
  assert(ctx.getObjectCode());

  Linker linker(GetBuiltinFunctionMap(), GetBuiltinClassMap());
  vector<ObjectCode*> objs;
  ctx.getProgramRoot()->collectObjectCode(objs);

  // find the objcode corresponding to ctx's objcode
  vector<ObjectCode*>::iterator pos =
    find(objs.begin(), objs.end(), ctx.getObjectCode());
  assert(pos != objs.end());

  Executable *exec = linker.link(objs, pos - objs.begin());
  ExecutionContext execCtx(exec);
  ExecutionContext::DefaultCallback callback;
  execCtx.execute(callback);
  delete exec;
}

bool compile_and_exec(const string& fname, compile_result& result) {
  result.result  = compile_result::Success;
  result.message = "";

  fstream infile(fname.c_str());
  if (!infile.good()) {
    throw invalid_argument("Invalid filename: " + fname);
  }

  SemanticContext ctx("<prelude>");
  // ctx takes ownership of root symbols
  NewBootstrapSymbolTable(&ctx);

  try {
    SemanticContext *mainCtx =
      ctx.newChildContext(util::strip_extension(fname));
    unsafe_compile(fname, infile, *mainCtx);
    if (global_compile_opts.semantic_check_only) return true;
    exec(*mainCtx);
  } catch (ParseErrorException& e) {
    result.result  = compile_result::InvalidSyntax;
    result.message = string("Syntax Error: ") + e.what();
    return false;
  } catch (SemanticViolationException& e) {
    result.result  = compile_result::SemanticError;
    result.message = string("Semantic Violation: ") + e.what();
    return false;
  } catch (TypeViolationException& e) {
    result.result  = compile_result::TypeError;
    result.message = string("Typecheck Violation: ") + e.what();
    return false;
  } catch (exception& e) {
    result.result  = compile_result::UnknownError;
    result.message = string("Uncaught Exception: ") + e.what();
    return false;
  }
  return true;
}


} // namespace venom
