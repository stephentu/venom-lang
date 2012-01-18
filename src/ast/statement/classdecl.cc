#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

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
    dynamic_cast<StmtListNode*>(stmts)->appendStatement(ctor);

    ctor->initSymbolTable(stmts->getSymbolTable());
    ctor->registerSymbol(ctx);
    ctor->semanticCheckImpl(ctx, false); // technically not needed,
                                         // since empty body
  }
  assert(stmts->getLocationContext() & TopLevelClassBody);
  assert(stmts->getSymbolTable()->findFuncSymbol("<ctor>", SymbolTable::NoRecurse, t));
}

void
ClassDeclNode::collectInstantiatedTypes(vector<InstantiatedType*>& types) {
  vector<InstantiatedType*> parents = getParents();
  for (vector<InstantiatedType*>::iterator it = parents.begin();
       it != parents.end(); ++it) {
    if ((*it)->isSpecializedType()) types.push_back(*it);
  }
  ASTNode::collectInstantiatedTypes(types);
}

void
ClassDeclNode::codeGen(CodeGenerator& cg) {
  // if this is a parameterized class decl, then skip code gen
  vector<InstantiatedType*> typeParams = getTypeParams();
  if (!typeParams.empty()) return;

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

  // link the stmts symbol table to the parents symbol tables
  for (vector<InstantiatedType*>::const_iterator it = parentTypes.begin();
       it != parentTypes.end(); ++it) {
    TypeMap map;
    // we only do this check so we can avoid failing the assert() for now,
    // for builtin classes
    if (!(*it)->getParams().empty()) {
      // build the type map
      ClassDeclNode *pcdn =
        dynamic_cast<ClassDeclNode*>((*it)->getClassSymbolTable()->getOwner());
      // TODO: this assert won't work for built-ins
      assert(pcdn);
      // TODO: STL style
      assert(pcdn->getTypeParams().size() == (*it)->getParams().size());
      for (size_t i = 0; i < pcdn->getTypeParams().size(); i++) {
        map.push_back(
            InstantiatedTypePair(pcdn->getTypeParams()[i],
                                 (*it)->getParams()[i]));
      }
    }
    stmts->getSymbolTable()->addClassParent(
        (*it)->getClassSymbolTable(), map);
  }
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
        ctx->getRootSymbolTable()->newChildScope(NULL),
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
ClassDeclNodeParser::cloneImpl() {
  return new ClassDeclNodeParser(
      name,
      util::transform_vec(parents.begin(), parents.end(),
        ParameterizedTypeString::CloneFunctor()),
      typeParams, stmts->clone());
}

ClassDeclNode*
ClassDeclNodeParser::cloneForTemplateImpl(const TypeTranslator& t) {
  // assert that we have already registered this symbol
  assert(parents.empty() ?
           parentTypes.size() == 1 :
           parents.size() == parentTypes.size());
  assert(typeParamTypes.size() == typeParams.size());

  // assert that the type translator completely instantiates
  // this class's type
  BaseSymbol* bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(ClassSymbol, bs);
  ClassSymbol* cs = static_cast<ClassSymbol*>(bs);
  SemanticContext* ctx = getSymbolTable()->getSemanticContext();
  InstantiatedType* itype =
    t.translate(ctx, cs->getSelfType(ctx));
  InstantiatedType::AssertNoTypeParamPlaceholders(itype);

  return new ClassDeclNodeSynthetic(
      itype->createClassName(),
      util::transform_vec(
        parentTypes.begin(), parentTypes.end(),
        TypeTranslator::TranslateFunctor(ctx, t)),
      InstantiatedTypeVec(),
      stmts->cloneForTemplate(t),
      itype);
}

}
}
