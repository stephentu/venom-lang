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

#ifndef VENOM_AST_EXPRLIST_H
#define VENOM_AST_EXPRLIST_H

#include <ast/expression/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class ExprListNode : public ASTExpressionNode {
public:
  ExprListNode() {}
  ExprListNode(const ExprNodeVec& exprs) : exprs(exprs) {}

  ~ExprListNode() {
    util::delete_pointers(exprs.begin(), exprs.end());
  }

  /** Takes ownership of expr. Returns self for chaining */
  inline ExprListNode* prependExpression(ASTExpressionNode* expr) {
    exprs.insert(exprs.begin(), expr);
    return this;
  }

  /** Takes ownership of expr. Returns self for chaining */
  inline ExprListNode* appendExpression(ASTExpressionNode* expr) {
    exprs.push_back(expr);
    return this;
  }

  virtual size_t getNumKids() const { return exprs.size(); }

  virtual ASTNode* getNthKid(size_t kid) { return exprs.at(kid); }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, exprs.size());
    VENOM_SAFE_SET_EXPR(exprs[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, exprs.size());
    return false;
  }

  virtual analysis::BaseSymbol* getSymbol() {
    return exprs[exprs.size() - 1]->getSymbol();
  }

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(ExprListNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(exprs" << std::endl << util::indent(indent + 1);
    PrintExprNodeVec(o, exprs, indent + 1);
    o << ")";
  }

private:
  ExprNodeVec exprs;
};

}
}

#endif /* VENOM_AST_EXPRLIST_H */
