#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/expression/synthetic/variable.h>

#include <ast/statement/classattrdecl.h>
#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtlist.h>

#include <ast/statement/synthetic/classdecl.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
ClassDeclNode::getSelfType(SemanticContext* ctx) {
  InstantiatedType* itype = getInstantiationOfType();
  if (itype) return itype;
  TypeTranslator t;
  ClassSymbol *cs = getSymbolTable()->findClassSymbol(
        getName(), SymbolTable::NoRecurse, t);
  assert(cs);
  return cs->getSelfType(ctx);
}

void
ClassDeclNode::registerSymbol(SemanticContext* ctx) {
  // check to see if this class is already defined in this scope
  if (symbols->isDefined(name, SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Class " + name + " already defined");
  }

  // type params
  checkAndInitTypeParams(ctx);

  // parents
  checkAndInitParents(ctx);
  vector<InstantiatedType*> parents = getParents();

  assert(parents.size() >= 1);
  if (parents.size() > 1) {
    throw SemanticViolationException(
        "Multiple inheritance currently not supported");
  }

  registerClassSymbol(ctx, parents, getTypeParams());
}

void
ClassDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }

  // register all the children first
  forchild (kid) {
    kid->registerSymbol(ctx);
  } endfor

  // now recurse on children w/o registration
  forchild (kid) {
    kid->semanticCheckImpl(ctx, false);
  } endfor

  // now look for a ctor definition
  TypeTranslator t;
  if (!stmts->getSymbolTable()->findFuncSymbol("<ctor>", SymbolTable::NoRecurse, t)) {
    // no ctor defined, insert a default one
    CtorDeclNode *ctor =
      new CtorDeclNode(ExprNodeVec(), new StmtListNode, ExprNodeVec());
    VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
    static_cast<StmtListNode*>(stmts)->appendStatement(ctor);

    ctor->initSymbolTable(stmts->getSymbolTable());
    ctor->registerSymbol(ctx);
    ctor->semanticCheckImpl(ctx, false); // technically not needed,
                                         // since empty body
  }
  assert(stmts->getLocationContext() & TopLevelClassBody);
  assert(stmts->getSymbolTable()->findFuncSymbol(
          "<ctor>", SymbolTable::NoRecurse, t));
}

void
ClassDeclNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  vector<InstantiatedType*> parents = getParents();
  for (vector<InstantiatedType*>::iterator it = parents.begin();
       it != parents.end(); ++it) {
    InstantiatedType* itype = t.translate(ctx, *it);
    if (itype->isSpecializedType()) callback.offerType(itype);
  }
  // don't recurse unless this class decl is instantiated
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, getSymbol());
  ClassSymbol* csym = static_cast<ClassSymbol*>(getSymbol());
  if (t.translate(ctx, csym->getSelfType(ctx))->isFullyInstantiated()) {
    ASTNode::collectSpecialized(ctx, t, callback);
  }
}

void
ClassDeclNode::liftPhaseImpl(SemanticContext* ctx,
                             SymbolTable* liftInto,
                             vector<ASTStatementNode*>& liftedStmts) {
  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);
  static_cast<StmtListNode*>(stmts)->liftRecurseAndInsert(ctx);
}

void
ClassDeclNode::codeGen(CodeGenerator& cg) {
  assert(!isTypeParameterized());

  // otherwise, continue as usual
  ASTStatementNode::codeGen(cg);
}

BaseSymbol*
ClassDeclNode::getSymbol() {
  TypeTranslator t;
  return symbols->findClassSymbol(name, SymbolTable::NoRecurse, t);
}

void
ClassDeclNode::createClassSymbol(
    const string& name,
    SymbolTable* classTable,
    Type* type,
    const vector<InstantiatedType*>& typeParams) {
  symbols->createClassSymbol(name, classTable, type, typeParams);
}

void
ClassDeclNode::registerClassSymbol(
    SemanticContext* ctx,
    const vector<InstantiatedType*>& parentTypes,
    const vector<InstantiatedType*>& typeParamTypes) {

  // Define this class's symbol. This MUST happen before semantic checks
  // on the children (to support self-references)
  // TODO: support multiple inheritance
  Type *type =
    ctx->createType(name, parentTypes.front(), typeParamTypes.size());
  createClassSymbol(name, stmts->getSymbolTable(),
                    type, typeParamTypes);
}

ASTStatementNode*
ClassDeclNode::cloneForLiftImplHelper(LiftContext& ctx) {
  assert(!isTypeParameterized());
  assert(ctx.isLiftingClass());
  assert(ctx.curLiftSym == getSymbol());

  ASTStatementNode* stmtsClone = stmts->cloneForLift(ctx);
  VENOM_ASSERT_TYPEOF_PTR(StmtListNode, stmts);

  // now add ref versions of lifted symbols into
  // class body definition
  for (vector<BaseSymbol*>::iterator it = ctx.refs.vec.begin();
       it != ctx.refs.vec.end(); ++it) {
    VENOM_ASSERT_TYPEOF_PTR(Symbol, *it);
    assert((*it)->getDefinedSymbolTable() == ctx.definedIn);
    Symbol* s = static_cast<Symbol*>(*it);
    InstantiatedType* refType =
      s->getInstantiatedType()->refify(ctx.definedIn->getSemanticContext());
    ClassAttrDeclNode *cattr = new ClassAttrDeclNode(
        new VariableNodeSynthetic(ctx.refParamName(s), refType),
        NULL);
    static_cast<StmtListNode*>(stmtsClone)->prependStatement(cattr);
  }

  return new ClassDeclNodeSynthetic(
      ctx.liftedName,
      getParents(),
      InstantiatedTypeVec(),
      stmtsClone);
}

struct functor {
  functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline
  InstantiatedType* operator()(const ParameterizedTypeString* t) const {
    return ctx->instantiateOrThrow(st, t);
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

void
ClassDeclNodeParser::print(ostream& o, size_t indent) {
  o << "(class " << name << std::endl << util::indent(indent + 1);
  o << "(type-params (" <<
    util::join(typeParams.begin(), typeParams.end(), ",") <<
    "))" << std::endl << util::indent(indent + 1);
  stmts->print(o, indent + 1);
  o << ")";
}

void
ClassDeclNodeParser::checkAndInitTypeParams(SemanticContext* ctx) {
  // type params
  assert(typeParamTypes.empty());
  typeParamTypes.reserve(typeParams.size());
  for (size_t pos = 0; pos < typeParams.size(); pos++) {
    // add all the type params into the body's symtab
    Type *type = ctx->createTypeParam(typeParams[pos], pos);
    typeParamTypes.push_back(type->instantiate(ctx));
    stmts->getSymbolTable()->createClassSymbol(
        typeParams[pos],
        ctx->getRootSymbolTable()->newChildScopeNoNode(),
        type);
  }
}

void
ClassDeclNodeParser::checkAndInitParents(SemanticContext* ctx) {
  // check to see if all parents are defined
  // use stmts's symtab to capture the parameterized types
  assert(parentTypes.empty());
  parentTypes.resize(parents.size());
  transform(parents.begin(), parents.end(),
            parentTypes.begin(), functor(ctx, stmts->getSymbolTable()));
  if (parents.empty()) {
    // if no explicit parents declared, parent is object
    parentTypes.push_back(InstantiatedType::ObjectType);
  }
}

ClassDeclNode*
ClassDeclNodeParser::cloneImpl(CloneMode::Type type) {
	switch (type) {
	case CloneMode::Structural:
		return new ClassDeclNodeParser(
				name,
				util::transform_vec(parents.begin(), parents.end(),
					ParameterizedTypeString::CloneFunctor()),
				typeParams, stmts->clone(type));
	case CloneMode::Semantic:
		assert(parents.empty() ?
						 parentTypes.size() == 1 :
						 parents.size() == parentTypes.size());
		assert(typeParamTypes.size() == typeParams.size());
		return new ClassDeclNodeSynthetic(
				name,
				parentTypes,
				typeParamTypes, stmts->clone(type));
	default: VENOM_NOT_REACHED;
	}
}

ASTStatementNode*
ClassDeclNodeParser::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

ClassDeclNode*
ClassDeclNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  // assert that we have already registered this symbol
  assert(parents.empty() ?
           parentTypes.size() == 1 :
           parents.size() == parentTypes.size());
  assert(typeParamTypes.size() == typeParams.size());

  BaseSymbol* bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, bs);
  ClassSymbol* cs = static_cast<ClassSymbol*>(bs);
  SemanticContext* ctx = getSymbolTable()->getSemanticContext();
  InstantiatedType* itype =
    t.translate(ctx, cs->getSelfType(ctx));

  // TODO: assert only two cases:
  //   1) full type instantiation
  //   2) no instantiations
  // In other words, no partial instantiations allowed

  if (itype->isSpecializedType()) {
    // instantiated type
    InstantiatedType::AssertNoTypeParamPlaceholders(itype);
    return new ClassDeclNodeSynthetic(
        itype->createClassName(),
        util::transform_vec(
          parentTypes.begin(), parentTypes.end(),
          TypeTranslator::TranslateFunctor(ctx, t)),
        InstantiatedTypeVec(),
        stmts->cloneForTemplate(t),
        itype);
  } else {
    TypeTranslator tt = t;

    // TODO: this is copied from
    //    ast/statement/funcdecl.cc
    // clone the type param types, so we can create new
    // ClassSymbols for them
    vector<InstantiatedType*> newTypeParamTypes;
    newTypeParamTypes.reserve(typeParamTypes.size());
    for (vector<InstantiatedType*>::iterator it = typeParamTypes.begin();
         it != typeParamTypes.end(); ++it) {
      VENOM_ASSERT_TYPEOF_PTR(TypeParamType, (*it)->getType());
      TypeParamType* t = static_cast<TypeParamType*>((*it)->getType());
      Type* t0 = ctx->createTypeParam(t->getName(), t->getPos());
      newTypeParamTypes.push_back(t0->instantiate(ctx));

      // add to type translator
      tt.addTranslation(*it, newTypeParamTypes.back());
    }

    // regular
    return new ClassDeclNodeSynthetic(
        name,
        util::transform_vec(
          parentTypes.begin(), parentTypes.end(),
          TypeTranslator::TranslateFunctor(ctx, tt)),
        newTypeParamTypes,
        stmts->cloneForTemplate(tt),
        NULL);
  }
}

}
}
