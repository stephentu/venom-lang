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

#ifndef VENOM_AST_DICTLITERAL_H
#define VENOM_AST_DICTLITERAL_H

#include <vector>
#include <utility>

#include <ast/expression/node.h>
#include <util/macros.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class DictPair :
  public ASTExpressionNode,
  public std::pair< ASTExpressionNode*, ASTExpressionNode* > {
public:
  DictPair(ASTExpressionNode* key, ASTExpressionNode* value)
    : std::pair< ASTExpressionNode*, ASTExpressionNode* >(key, value) {}

  ~DictPair() {
    delete first;
    delete second;
  }

  inline ASTExpressionNode* key()   { return first;  }
  inline ASTExpressionNode* value() { return second; }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {first, second};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 2);
    VENOM_SAFE_SET2(first, second, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 2);
    return false;
  }

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return ASTNode::rewriteLocal(ctx, mode);
  }

  virtual void codeGen(backend::CodeGenerator& cg) {
    // is re-written, so never need to code-generate it
    VENOM_NOT_REACHED;
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(DictPair)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs) {
    VENOM_UNIMPLEMENTED;
  }

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(pair ";
    key()->print(o, indent);
    o << " ";
    value()->print(o, indent);
    o << ")";
  }

};

typedef std::vector<DictPair*> DictPairVec;

inline DictPairVec* MakeDictPairVec1(DictPair* pair) {
  DictPairVec *v = new DictPairVec;
  v->push_back(pair);
  return v;
}

class DictLiteralNode : public ASTExpressionNode {
public:

  /** Takes ownership of the nodes in the pairs */
  DictLiteralNode(const DictPairVec& pairs)
    : pairs(pairs) {}

  ~DictLiteralNode() {
    util::delete_pointers(pairs.begin(), pairs.end());
  }

  virtual size_t getNumKids() const { return pairs.size(); }

  virtual ASTNode* getNthKid(size_t kid) {
    VENOM_CHECK_RANGE(kid, pairs.size());
    return pairs[kid];
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, pairs.size());
    VENOM_SAFE_SET_EXPR(pairs[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, pairs.size());
    return false;
  }

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg) {
    // is re-written, so never need to code-generate it
    VENOM_NOT_REACHED;
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(DictLiteralNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(dictliteral ";
    // TODO: abstract this away with Print{Expr,Stmt}NodeVec
    for (DictPairVec::iterator it = pairs.begin();
         it != pairs.end(); ++it) {
      (*it)->print(o, indent);
      if (it + 1 != pairs.end()) o << " ";

    }
    o << ")";
  }

private:
  DictPairVec pairs;
};

}
}

#endif /* VENOM_AST_DICTLITERAL_H */

