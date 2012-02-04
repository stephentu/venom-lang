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
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
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

#ifndef VENOM_AST_IFSTMT_H
#define VENOM_AST_IFSTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class IfStmtNode : public ASTStatementNode {
public:
  /** Takes ownership of cond, true_branch, false_branch */
  IfStmtNode(ASTExpressionNode* cond,
             ASTStatementNode*  true_branch,
             ASTStatementNode*  false_branch)
    : cond(cond), true_branch(true_branch), false_branch(false_branch) {}

  ~IfStmtNode() {
    delete cond;
    delete true_branch;
    delete false_branch;
  }

  virtual size_t getNumKids() const { return 3; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {cond, true_branch, false_branch};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 3);
    VENOM_SAFE_SET3(cond, true_branch, false_branch, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 3);
    return k != 0;
  }

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual ASTNode* rewriteReturn(analysis::SemanticContext* ctx);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(IfStmtNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(if ";
    cond->print(o, indent);
    o << std::endl << util::indent(indent + 1);
    true_branch->print(o, indent + 1);
    o << std::endl << util::indent(indent + 1);
    false_branch->print(o, indent + 1);
    o << ")";
  }

private:
  ASTExpressionNode* cond;
  ASTStatementNode*  true_branch;
  ASTStatementNode*  false_branch;
};

}
}

#endif /* VENOM_AST_IFSTMT_H */
