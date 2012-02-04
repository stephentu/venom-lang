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

#ifndef VENOM_AST_SYNTHETIC_FUNCTIONCALL_H
#define VENOM_AST_SYNTHETIC_FUNCTIONCALL_H

#include <ast/expression/functioncall.h>

namespace venom {
namespace ast {

class FunctionCallNodeSynthetic : public FunctionCallNode {
public:
  FunctionCallNodeSynthetic(
      ASTExpressionNode* primary,
      const std::vector<analysis::InstantiatedType*>& typeArgTypes,
      const ExprNodeVec& args)
    : FunctionCallNode(primary, args), typeArgTypes(typeArgTypes) {}

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeArgTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(FunctionCallNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) {}

private:
  std::vector<analysis::InstantiatedType*> typeArgTypes;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_FUNCTIONCALL_H */
