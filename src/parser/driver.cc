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
#include <util/graph.h>

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

struct _itype_less_cmp {
  inline bool operator()(const InstantiatedType* lhs,
                         const InstantiatedType* rhs) const {
    return lhs->stringify() < rhs->stringify();
  }
};

//typedef util::directed_graph<InstantiatedType*, _itype_less_cmp>
//        dep_graph;
//
//static void recursiveInsert(InstantiatedType* type, dep_graph& depGraph) {
//  assert(type->isSpecializedType());
//  InstantiatedType::AssertNoTypeParamPlaceholders(type);
//
//  if (type->getType()->getParent()->getParams().empty()) {
//    // if this type does not depend on a parameterized type
//    // then just add it
//    depGraph.add_elem(type);
//  } else {
//    InstantiatedType* parentType = type->getParentInstantiatedType();
//    InstantiatedType::AssertNoTypeParamPlaceholders(parentType);
//    // add edge from type -> parent
//    depGraph.add_edge(type, parentType);
//
//    // recurse on parent
//    recursiveInsert(parentType, depGraph);
//  }
//}
//
//class collector : public ASTNode::CollectCallback {
//public:
//  virtual void offer(InstantiatedType* type) {
//    assert(type->isSpecializedType());
//    recursiveInsert(type, graph);
//    if (type->getClassSymbolTable()->getOwner()) {
//      TypeTranslator t;
//      t.bind(type);
//      type->getClassSymbolTable()->getOwner()->collectInstantiatedTypes(
//          type->getClassSymbolTable()->getSemanticContext(),
//          t, *this);
//    }
//  }
//
//  dep_graph graph;
//};

typedef set<InstantiatedType*, _itype_less_cmp> itype_set;

class collector : public ASTNode::CollectCallback {
public:
  collector() : itypes(_itype_less_cmp()) {}
  virtual void offer(InstantiatedType* type) {
    assert(type->isSpecializedType());
    itypes.insert(type);
  }
  itype_set itypes;
};

struct _collect_types_functor {
  _collect_types_functor(ASTNode::CollectCallback& callback)
    : callback(&callback) {}
  inline void operator()(ASTStatementNode* root, SemanticContext* ctx) const {
    TypeTranslator t;
    root->collectInstantiatedTypes(ctx, t, *callback);
  }
  inline void operator()(
      pair<ASTStatementNode*, SemanticContext*>& p) const {
    operator()(p.first, p.second);
  }
  ASTNode::CollectCallback* callback;
};

#define _IMPL_PRINT_AST(rootNode, stagename) \
  do { \
    if (global_compile_opts.print_ast) { \
      cerr << "After stage (" << (stagename) << "):" << endl; \
      (rootNode)->print(cerr); \
      cerr << endl; \
    } \
  } while (0)

struct _lift_functor {
  inline void operator()(ASTStatementNode* root,
                         SemanticContext* ctx) const {
    VENOM_ASSERT_TYPEOF_PTR(StmtListNode, root);
    StmtListNode* stmtList = static_cast<StmtListNode*>(root);
    stmtList->liftPhase(ctx);
    _IMPL_PRINT_AST(root, "lift");
  }
};

#define _REWRITE_LOCAL_STAGES(x) \
  x(DeSugar) \
  x(CanonicalRefs) \
  x(ModuleMain) \
  x(FunctionReturns) \
  x(BoxPrimitives)

#define _IMPL_REWRITE_FUNCTOR(stage) \
  struct _rewrite_functor_##stage { \
    inline void operator()(ASTStatementNode* root, \
                           SemanticContext* ctx) const { \
      root->rewriteLocal(ctx, ASTNode::stage); \
      _IMPL_PRINT_AST(root, "rewriteLocal: " #stage); \
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

  { // begin template phase

    // resolve all specialized classes / instantiation phase
    // WARNING: this is tricky to get right

    SemanticContext::ModuleVec workingSet;
    ctx.getProgramRoot()->getAllModules(workingSet);

    itype_set processedAlready((_itype_less_cmp()));

    // maps (parameterized type -> vector of instantiated ast nodes)
    typedef map<InstantiatedType*, vector<ClassDeclNode*>, _itype_less_cmp>
            TypeNodeMap;
    TypeNodeMap specializedTypes((_itype_less_cmp()));

    while (true) {

      // traverse the program and gather all types
      collector callback;
      for_each(workingSet.begin(), workingSet.end(),
               _collect_types_functor(callback));

      //cerr << "collected: " <<
      //util::debug_stringify_ptr_coll(types.begin(), types.end(), ",") << endl;

      // filter out the ones already processed
      // (necessary to prevent infinite loops)
      vector<InstantiatedType*> typesToProcess;
      typesToProcess.reserve(callback.itypes.size());
      for (itype_set::iterator it = callback.itypes.begin();
           it != callback.itypes.end(); ++it) {
        if (processedAlready.find(*it) == processedAlready.end()) {
          typesToProcess.push_back(*it);
        }
      }

      //cerr << "typesToProcess: " <<
      //util::debug_stringify_ptr_coll(typesToProcess.begin(), end_it, ",")
      //<< endl;

      // if no types to process, then we are done
      if (typesToProcess.empty()) break;

      // otherwise, mark that we are processing these types
      processedAlready.insert(typesToProcess.begin(), typesToProcess.end());

      vector<ClassDeclNode*> classDecls;
      classDecls.reserve(typesToProcess.size());
      for (vector<InstantiatedType*>::iterator it = typesToProcess.begin();
           it != typesToProcess.end(); ++it) {
        InstantiatedType* itype = *it;
        assert(itype->isSpecializedType());

        ASTNode* node = itype->getClassSymbolTable()->getOwner();
        if (node) {
          // user-class case

          VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, node);
          ClassDeclNode *classNode = static_cast<ClassDeclNode*>(node);

          // do the type instantiation
          TypeTranslator t;
          t.bind(itype);

          // instantiate
          ClassDeclNode* instantiation =
            ASTNode::CloneForTemplate(classNode, t);

          // process the new instantiation
          SymbolTable* scope =
            itype->getClassSymbol()->getDefinedSymbolTable();
          instantiation->initSymbolTable(scope);
          instantiation->semanticCheck(scope->getSemanticContext());
          instantiation->typeCheck(scope->getSemanticContext());

          // inserto type into map
          InstantiatedType* selfType =
            itype->getClassSymbol()->getSelfType(scope->getSemanticContext());
          specializedTypes[selfType].push_back(instantiation);

          classDecls.push_back(instantiation);
        } else {
          // builtin special case - for types belonging to <prelude>, we handle
          // it separately (since it is not associated with some AST node). in
          // the future, when we support native user extensions, then we will
          // also have to include that here
          SemanticContext* rootCtx = ctx.getProgramRoot();
          SymbolTable* rootTable = rootCtx->getRootSymbolTable();
          TypeTranslator t;
          ClassSymbol* csym =
            rootTable->findClassSymbol(itype->createClassName(),
                SymbolTable::NoRecurse, t);
          if (!csym) {
            // need to create the specific instantiation
            TypeTranslator t;
            t.bind(itype);
            itype->getClassSymbol()->instantiateSpecializedType(t);
          }
        }
      }

      workingSet.clear();
      workingSet.reserve(classDecls.size());
      for (vector<ClassDeclNode*>::iterator it = classDecls.begin();
           it != classDecls.end(); ++it) {
        workingSet.push_back(
            make_pair(*it, (*it)->getSymbolTable()->getSemanticContext()));
      }
    }

    // insert the create ast nodes alongside the original template ast
    // node
    for (TypeNodeMap::iterator it = specializedTypes.begin();
         it != specializedTypes.end(); ++it) {
      InstantiatedType* origType = it->first;
      vector<ClassDeclNode*>& astNodes = it->second;

      // assert not a builtin class (is user defined)
      assert(origType->getClassSymbolTable()->getOwner());

      ASTNode* owner =
        origType->getClassSymbol()->getDefinedSymbolTable()->getOwner();
      if (!owner) {
        // is defined as a top level class
        // TODO: this is kind of hacky...
        owner = origType
            ->getClassSymbolTable()
            ->getSemanticContext()
            ->getModuleRoot();
      }
      VENOM_ASSERT_TYPEOF_PTR(StmtListNode, owner);
      static_cast<StmtListNode*>(owner)
        ->insertSpecializedTypes(origType, astNodes);
    }

  } // end template phase

  // lift phase
  ctx.getProgramRoot()->forEachModule(_lift_functor());

  // local rewrite phases

#define _IMPL_REWRITE_LOCAL(stage) \
  do { \
    ctx.getProgramRoot()->forEachModule(_rewrite_functor_##stage()); \
  } while (0);

  _REWRITE_LOCAL_STAGES(_IMPL_REWRITE_LOCAL)

#undef _IMPL_REWRITE_LOCAL

  if (global_compile_opts.semantic_check_only) return;

  // code gen phase
  ctx.getProgramRoot()->forEachModule(_codegen_functor());
}

#undef _REWRITE_LOCAL_STAGES
#undef _IMPL_PRINT_AST

static void exec(SemanticContext& ctx) {
  assert(ctx.getObjectCode());

  Linker linker(
      GetBuiltinFunctionMap(ctx.getProgramRoot()),
      GetBuiltinClassMap(ctx.getProgramRoot()));

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
