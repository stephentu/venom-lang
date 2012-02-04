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

#ifndef VENOM_AST_EXPRESSION_NODE_H
#define VENOM_AST_EXPRESSION_NODE_H

#include <iostream>
#include <string>
#include <vector>

#include <ast/node.h>
#include <util/stl.h>

namespace venom {

namespace analysis {
  /** Forward decl for typeCheck */
  class InstantiatedType;
  typedef std::vector<InstantiatedType*> InstantiatedTypeVec;
}

namespace ast {

/** This contains the structure of an un-resolved type,
 * which we get from the parser. The reason we don't use
 * Type objects right away is because Type objects represent
 * properly resolved types, which we cannot get right away
 * from the parser */
struct ParameterizedTypeString {
  ParameterizedTypeString(const util::StrVec& names)
    : names(names) {}
  ParameterizedTypeString(const util::StrVec& names,
                          const std::vector<ParameterizedTypeString*>& params)
    : names(names), params(params) {}
  ~ParameterizedTypeString() {
    util::delete_pointers(params.begin(), params.end());
  }

  std::string stringify() const;

  ParameterizedTypeString* clone();
  struct CloneFunctor {
    typedef ParameterizedTypeString* result_type;
    inline ParameterizedTypeString*
		operator()(ParameterizedTypeString* ptr) const { return ptr->clone(); }
  };

  struct StringerFunctor :
    public util::stringify_functor<ParameterizedTypeString>::ptr {};

  const std::vector<std::string>              names;
  const std::vector<ParameterizedTypeString*> params;
};

typedef std::vector<ParameterizedTypeString*> TypeStringVec;

class ASTExpressionNode : public ASTNode {
public:
  ASTExpressionNode() : staticType(NULL), expectedType(NULL) {}

  inline analysis::InstantiatedType*
    getStaticType() { return staticType; }
  inline const analysis::InstantiatedType*
    getStaticType() const { return staticType; }

  inline analysis::InstantiatedType*
    getExpectedType() { return expectedType; }
  inline const analysis::InstantiatedType*
    getExpectedType() const { return expectedType; }

  inline analysis::InstantiatedTypeVec&
    getTypeParamArgs() { return typeParams; }
  inline const analysis::InstantiatedTypeVec&
    getTypeParamArgs() const { return typeParams; }

  /** Do type-checking on this node recursively.
   *  expected is what you expect this node to type-check to, if it matters
   *    (is NULL in most cases)
   *  typeParamArgs is a list of type parameter arguments, if it matters
   *    (is empty in most cases)
   */
  analysis::InstantiatedType*
    typeCheck(analysis::SemanticContext* ctx,
              analysis::InstantiatedType* expected = NULL,
              const analysis::InstantiatedTypeVec& typeParamArgs
                = analysis::InstantiatedTypeVec()) {
    return staticType =
      typeCheckImpl(ctx, expectedType = expected, typeParams = typeParamArgs);
  }

  virtual void collectSpecialized(
      analysis::SemanticContext* ctx,
      const analysis::TypeTranslator& t,
      CollectCallback& callback);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  VENOM_AST_TYPED_CLONE_EXPR(ASTExpressionNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs) = 0;

  virtual ASTExpressionNode*
    replace(analysis::SemanticContext* ctx, ASTNode* replacement);

  analysis::InstantiatedType* staticType;
  analysis::InstantiatedType* expectedType;
  analysis::InstantiatedTypeVec typeParams;
};

typedef std::vector<ASTExpressionNode *> ExprNodeVec;

inline ExprNodeVec * MakeExprVec1(ASTExpressionNode *a0) {
  ExprNodeVec *v = new ExprNodeVec;
  v->push_back(a0);
  return v;
}

inline void PrintExprNodeVec(std::ostream& o,
                             const ExprNodeVec& exprs,
                             size_t indent) {
  o << "(";
  for (ExprNodeVec::const_iterator it = exprs.begin();
       it != exprs.end(); ++it) {
    (*it)->print(o, indent);
    if (it + 1 != exprs.end()) o << " ";
  }
  o << ")";
}

}
}

inline std::ostream& operator<<(std::ostream& o,
                                const venom::ast::ParameterizedTypeString& t) {
  o << t.stringify();
  return o;
}

#endif /* VENOM_AST_EXPRESSION_NODE_H */
