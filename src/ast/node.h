#ifndef VENOM_AST_NODE_H
#define VENOM_AST_NODE_H

#include <cassert>
#include <iostream>

#include <util/macros.h>

namespace venom {

namespace analysis {
  /** Forward decls **/
  class BaseSymbol;
  class SemanticContext;
  class SymbolTable;
}

namespace backend {
  /** Forward decls **/
  class CodeGenerator;
}

namespace ast {

class FuncDeclNode;
class ClassDeclNode;

template <typename T>
struct _CloneFunctor {
  typedef T* result_type;
  inline T* operator()(T* ptr) const
    { T* ret = ptr->clone(); assert(ret); return ret; }
};

#define VENOM_AST_CLONE_FUNCTOR(type) \
  typedef _CloneFunctor<type> CloneFunctor;

class ASTNode {
  friend class StmtListNode;
public:
  ASTNode() : symbols(NULL), locCtx(0) {}

  virtual ~ASTNode();

  /** Tree Traversal **/

  virtual size_t getNumKids() const = 0;

  virtual ASTNode* getNthKid(size_t kid) = 0;
  inline const ASTNode* getNthKid(size_t kid) const {
    return const_cast<ASTNode*>(this)->getNthKid(kid);
  }

  /** Tree Mutators */

  virtual void setNthKid(size_t idx, ASTNode* kid) = 0;

  /** AST Context **/

  /* In expr(), expr has FunctionCall ctx */
  static const uint32_t FunctionCall      = 0x1;

  /* In class Foo stmtlist end, all top level stmts in stmtlist have
   * TopLevelClassBody ctx */
  static const uint32_t TopLevelClassBody = 0x1 << 1;

  /* In def fn(...) = stmtlist end, all top level
   * stmts in stmtlist have TopLevelFuncBody */
  static const uint32_t TopLevelFuncBody  = 0x1 << 2;

  /* top level exprs on the lhs of assign */
  static const uint32_t AssignmentLHS     = 0x1 << 3;

  /* parameter of function **declaration** (not call) */
  static const uint32_t FunctionParam     = 0x1 << 4;

  inline uint32_t getLocationContext() const         { return locCtx;      }
  inline bool hasLocationContext(uint32_t ctx) const { return locCtx & ctx;}

  virtual void setLocationContext(uint32_t ctx)      { locCtx = ctx;       }
  virtual void addLocationContext(uint32_t ctx)      { locCtx |= ctx;      }
  virtual void clearLocationContext(uint32_t ctx)    { locCtx &= ~ctx;     }

  FuncDeclNode* getEnclosingFuncNode();
  const FuncDeclNode* getEnclosingFuncNode() const;

  ClassDeclNode* getEnclosingClassNode();
  const ClassDeclNode* getEnclosingClassNode() const;

  /** Semantic checks **/

  inline analysis::SymbolTable*
    getSymbolTable() { return symbols; }
  inline const analysis::SymbolTable*
    getSymbolTable() const { return symbols; }

  /**
   * Returns true if the k-th child of this AST node represents
   * a new scope, false otherwise.
   */
  virtual bool needsNewScope(size_t k) const = 0;

  /** Call to set up the symbol tables recursively.
   * Must only call once (subsequent calls will trigger assert failures).
   * NOTE: ASTNodes do NOT take ownership of symbols */
  void initSymbolTable(analysis::SymbolTable* symbols);

  /** Cannot call until after type-checking has completed */
  virtual analysis::BaseSymbol* getSymbol() { VENOM_UNIMPLEMENTED; }

  /**
   * Perform semantic checks on this node (recursively)
   */
  void semanticCheck(analysis::SemanticContext* ctx) {
    semanticCheckImpl(ctx, true);
  }

  /** Main semanticCheck implementation. Should not be called
   * outside the AST node hierarchy
   * TODO: declare as protected, with appropriate friends
   */
  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  /**
   * Called when this node should register its symbol in its corresponding
   * symbol table. Note this is not intending to be recursive, so a node should
   * not worry about registering its children. Does nothing by default
   * TODO: declare as protected, with appropriate friends
   */
  virtual void registerSymbol(analysis::SemanticContext* ctx) {}

  /** Tree re-writing **/

  /** Order is important- rewrites must be run in this order
   * for correctness */
  enum RewriteMode {
    CanonicalRefs, // rewrite module level vars into attr access +
                   // un-qualified attrs x into self.x

    ModuleMain,    // rewrite module level statements into a <main>
                   // function

    FunctionReturns, // rewrite all stmt expr returns into explicit
                     // return statements

    BoxPrimitives, // box primitives when necessary
  };

  /**
   * Do a local tree rewrite on this node (recursively).
   *
   * If the return value is NULL, then that means no rewrite was performed.  If
   * the return value is not NULL, then that means a rewrite was performed, and
   * the return value is the *new* value of node. It is up to the caller to
   * delete the old node.
   */
  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  /** same return semantics as rewriteLocal */
  virtual ASTNode* rewriteReturn(analysis::SemanticContext* ctx);

  /** Code Generation **/

  /**
   * Generate code for this node recursively. If this node is an expression,
   * then leave the value of the expression at the top of the stack. If this
   * node is a statement, leave the stack intact (after computation).
   */
  virtual void codeGen(backend::CodeGenerator& cg);

  template <typename T>
  inline static T* Clone(T* node) {
    T* copy = node->clone();
    copy->setLocationContext(node->locCtx);
    return copy;
  };

  /** Should use the above Clone, instead of this one */
  virtual ASTNode* clone();

  VENOM_AST_CLONE_FUNCTOR(ASTNode)

#define VENOM_AST_TYPED_CLONE(type) \
  virtual type* clone() { return static_cast<type*>(ASTNode::clone()); } \
  VENOM_AST_CLONE_FUNCTOR(type)

/** Must be placed in the *public* section of the class decl */
#define VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(type) \
  VENOM_AST_TYPED_CLONE(type) \
  protected: \
  virtual type* cloneImpl(); \
  public:

  /** Debugging helpers **/

  /**
   * Print this AST node, and subsequent children, into ostream.
   * The contract is, when print() is called, the stream is ready
   * for printing. If a node needs to print a newline, it must
   * also print the necessary indents
   */
  virtual void print(std::ostream& o, size_t indent = 0) = 0;

  /**
   * Print to stdout, for debugging in gdb. Purposefully defined
   * const for maximum usefulness
   *
   * Definition is not in header, so it doesn't get inlined
   */
  void printStderr() const;

protected:

  /** Only sets *this* node's symbol table (non recursive) */
  inline void setSymbolTable(analysis::SymbolTable* symbols) {
    assert(!this->symbols);
    this->symbols = symbols;
  }

  /**
   * Replace this ast node with replacement. Meant to be called
   * during the rewriteLocal stage.
   */
  virtual ASTNode*
    replace(analysis::SemanticContext* ctx, ASTNode* replacement);

  /** Set any mutable state on the cloned node, which is not captured
   * by the constructor. Default does nothing */
  virtual void cloneSetState(ASTNode* node) {}

  /** Do the actual cloning */
  virtual ASTNode* cloneImpl() = 0;

  analysis::SymbolTable* symbols;
  uint32_t               locCtx;
};

}
}

#endif /* VENOM_AST_NODE_H */
