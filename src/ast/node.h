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

#ifndef VENOM_AST_NODE_H
#define VENOM_AST_NODE_H

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <util/container.h>
#include <util/macros.h>

namespace venom {

namespace analysis {
  /** Forward decls **/
  class BaseSymbol;
  class BoundFunction;
  class Symbol;
  class InstantiatedType;
  class SemanticContext;
  class SymbolTable;
  class TypeTranslator;
}

namespace backend {
  /** Forward decls **/
  class CodeGenerator;
}

namespace ast {

class ASTStatementNode;
class FuncDeclNode;
class ClassDeclNode;

/** This represents the lift of a particular statement */
struct LiftContext {
  typedef std::pair< std::vector<analysis::BaseSymbol*>, ASTStatementNode* >
          LiftMapEntry;

  typedef std::map<analysis::BaseSymbol*, LiftMapEntry>
          LiftMap;

  typedef util::container_pool<analysis::BaseSymbol*>
          LiftContainer;

  LiftContext(analysis::BaseSymbol* curLiftSym,
              const std::string& liftedName,
              analysis::SymbolTable* definedIn,
              const LiftMap& liftMap)
    : curLiftSym(curLiftSym), liftedName(liftedName),
      definedIn(definedIn), liftMap(liftMap) {}

  bool isLiftingFunction() const;
  bool isLiftingClass() const;

  std::string refParamName(analysis::Symbol* nonLocSym);

  static std::string RefParamName(analysis::Symbol* nonLocSym,
                                  size_t pos);

  analysis::BaseSymbol* const curLiftSym;
  const std::string liftedName;
  analysis::SymbolTable* const definedIn;
  const LiftMap liftMap;

  LiftContainer refs;
};

namespace {
  struct _CloneMode { enum Type { Structural, Semantic }; };

  template <typename T>
  struct _CloneFunctor {
    _CloneFunctor(_CloneMode::Type type) : type(type) {}
    typedef T* result_type;
    inline T* operator()(T* ptr) const { return ptr->clone(type); }
    _CloneMode::Type type;
  };

  template <typename T>
  struct _CloneTemplateFunctor {
    _CloneTemplateFunctor(const analysis::TypeTranslator& t) : t(&t) {}
    _CloneTemplateFunctor(const analysis::TypeTranslator* t) : t(t)  {}
    typedef T* result_type;
    inline T* operator()(T* ptr) const { return ptr->cloneForTemplate(*t); }
  private:
    const analysis::TypeTranslator* t;
  };

  template <typename InputNode, typename ResultNode>
  struct _CloneLiftFunctor {
    _CloneLiftFunctor(LiftContext& ctx) : ctx(&ctx) {}
    typedef ResultNode* result_type;
    inline ResultNode* operator()(InputNode* ptr) const {
      return ptr->cloneForLift(*ctx);
    }
  private:
    LiftContext* ctx;
  };

#define VENOM_AST_CLONE_FUNCTOR_IMPL(type, restype) \
  typedef _CloneFunctor<type> CloneFunctor; \
  typedef _CloneTemplateFunctor<type> CloneTemplateFunctor; \
  typedef _CloneLiftFunctor<type, restype> CloneLiftFunctor;

#define VENOM_AST_CLONE_FUNCTOR(type) \
  VENOM_AST_CLONE_FUNCTOR_IMPL(type, ASTNode)

#define VENOM_AST_CLONE_FUNCTOR_STMT(type) \
  VENOM_AST_CLONE_FUNCTOR_IMPL(type, ASTStatementNode)

#define VENOM_AST_CLONE_FUNCTOR_EXPR(type) \
  VENOM_AST_CLONE_FUNCTOR_IMPL(type, ASTExpressionNode)
}

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
  virtual void initSymbolTable(analysis::SymbolTable* symbols);

  /** Cannot call until after type-checking has completed */
  virtual analysis::BaseSymbol* getSymbol() { return NULL; }

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

  /** Templates **/

  struct CollectCallback {
    virtual void offerType(analysis::InstantiatedType* type) = 0;
    virtual void offerFunction(analysis::BoundFunction& func) = 0;
    virtual void offerMethod(
        analysis::InstantiatedType* klass,
        analysis::BoundFunction& method) = 0;
  };

  virtual void collectSpecialized(
      analysis::SemanticContext* ctx,
      const analysis::TypeTranslator& t,
      CollectCallback& callback);

  virtual bool isTypeParameterized() const { return false; }

  /** Tree re-writing **/

  virtual void collectNonLocalRefs(LiftContext& ctx);

  virtual ASTNode* rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const std::set<analysis::BaseSymbol*>& refs);

  /**
   * Order is important- rewrites must be run in this order
   * for correctness
   *
   * WARNING: IF you change this enum, must also change a
   * macro in parser/driver.cc
   */
  enum RewriteMode {
    DeSugar,       // rewrite list/dict literals into lower-level ops
                   // rewrite + -> concat for strings

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

  /** Cloning **/

  typedef _CloneMode CloneMode;

  /**
   * Use the clone static functions, when you will use the
   * cloned node in the same static (location) context as
   * the original node
   */

  template <typename T>
  inline static T* Clone(T* node, CloneMode::Type type) {
    T* copy = node->clone(type);
    copy->setLocationContext(node->locCtx);
    return copy;
  };

  template <typename T>
  inline static T* CloneForTemplate(
      T* node, const analysis::TypeTranslator& t) {
    T* copy = node->cloneForTemplate(t);
    copy->setLocationContext(node->locCtx);
    return copy;
  };

  template <typename From, typename To>
  inline static To* CloneForLift(From* node, LiftContext& ctx) {
    To* copy = node->cloneForLift(ctx);
    copy->setLocationContext(node->locCtx);
    return copy;
  };

  virtual ASTNode* clone(CloneMode::Type type);

  virtual ASTNode* cloneForTemplate(
      const analysis::TypeTranslator& translator);

  virtual ASTNode* cloneForLift(LiftContext& ctx);

  VENOM_AST_CLONE_FUNCTOR(ASTNode)

#define VENOM_AST_TYPED_CLONE_IMPL(type, restype) \
  virtual type* clone(CloneMode::Type t) \
    { return static_cast<type*>(ASTNode::clone(t)); } \
  virtual type* cloneForTemplate(const analysis::TypeTranslator& t) \
    { return static_cast<type*>(ASTNode::cloneForTemplate(t)); } \
  virtual restype* cloneForLift(LiftContext& ctx) \
    { return static_cast<restype*>(ASTNode::cloneForLift(ctx)); } \
  VENOM_AST_CLONE_FUNCTOR_IMPL(type, restype)

#define VENOM_AST_TYPED_CLONE_STMT(type) \
    VENOM_AST_TYPED_CLONE_IMPL(type, ASTStatementNode)

#define VENOM_AST_TYPED_CLONE_EXPR(type) \
    VENOM_AST_TYPED_CLONE_IMPL(type, ASTExpressionNode)

/** Must be placed in the *public* section of the class decl */
#define VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_IMPL(type, restype) \
  VENOM_AST_TYPED_CLONE_IMPL(type, restype) \
  protected: \
  virtual type* cloneImpl(CloneMode::Type type); \
  virtual type* cloneForTemplateImpl(const analysis::TypeTranslator& t); \
  virtual restype* cloneForLiftImpl(LiftContext& ctx); \
  public:

#define VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(type) \
    VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_IMPL(type, ASTStatementNode)

#define VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(type) \
    VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_IMPL(type, ASTExpressionNode)

  /** Debugging helpers **/

  /**
   * Print this AST node, and subsequent children, into ostream.
   * The contract is, when print() is called, the stream is ready
   * for printing. If a node needs to print a newline, it must
   * also print the necessary indents
   */
  virtual void print(std::ostream& o, size_t indent = 0) = 0;

  /**
   * Print to stderr, for debugging in gdb. Purposefully defined
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
  virtual ASTNode* cloneImpl(CloneMode::Type type) = 0;

  /** Do the actual cloning for template */
  virtual ASTNode* cloneForTemplateImpl(
      const analysis::TypeTranslator& t) = 0;

  /** Do the actual cloning for lift */
  virtual ASTNode* cloneForLiftImpl(LiftContext& ctx) = 0;

  analysis::SymbolTable* symbols;
  uint32_t               locCtx;
};

}
}

#endif /* VENOM_AST_NODE_H */
