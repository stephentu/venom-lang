#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <stack>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/type.h>

#include <ast/expression/boolliteral.h>
#include <ast/expression/doubleliteral.h>
#include <ast/expression/intliteral.h>
#include <ast/expression/nilliteral.h>
#include <ast/expression/node.h>

#include <util/macros.h>
#include <util/stl.h>

using namespace std;

using namespace venom::ast;

namespace venom {
namespace analysis {

Type* const Type::AnyType(new Type("any", NULL, NULL, 0));
InstantiatedType* const InstantiatedType::AnyType(Type::AnyType->instantiate());

/** Primitives **/
Type* const Type::IntType(new Type("int", NULL, InstantiatedType::AnyType, 0));
Type* const Type::BoolType(new Type("bool", NULL, InstantiatedType::AnyType, 0));
Type* const Type::FloatType(new Type("float", NULL, InstantiatedType::AnyType, 0));
Type* const Type::VoidType(new Type("void", NULL, InstantiatedType::AnyType, 0));

/** Objects **/
Type* const Type::ObjectType(new Type("object", NULL, InstantiatedType::AnyType, 0));
InstantiatedType* const InstantiatedType::ObjectType(Type::ObjectType->instantiate());

Type* const Type::Func0Type(new Type("func0", NULL, InstantiatedType::ObjectType, 1));
Type* const Type::Func1Type(new Type("func1", NULL, InstantiatedType::ObjectType, 2));
Type* const Type::Func2Type(new Type("func2", NULL, InstantiatedType::ObjectType, 3));
Type* const Type::Func3Type(new Type("func3", NULL, InstantiatedType::ObjectType, 4));
Type* const Type::Func4Type(new Type("func4", NULL, InstantiatedType::ObjectType, 5));
Type* const Type::Func5Type(new Type("func5", NULL, InstantiatedType::ObjectType, 6));
Type* const Type::Func6Type(new Type("func6", NULL, InstantiatedType::ObjectType, 7));
Type* const Type::Func7Type(new Type("func7", NULL, InstantiatedType::ObjectType, 8));
Type* const Type::Func8Type(new Type("func8", NULL, InstantiatedType::ObjectType, 9));
Type* const Type::Func9Type(new Type("func9", NULL, InstantiatedType::ObjectType, 10));

Type* const Type::Func10Type(new Type("func10", NULL, InstantiatedType::ObjectType, 11));
Type* const Type::Func11Type(new Type("func11", NULL, InstantiatedType::ObjectType, 12));
Type* const Type::Func12Type(new Type("func12", NULL, InstantiatedType::ObjectType, 13));
Type* const Type::Func13Type(new Type("func13", NULL, InstantiatedType::ObjectType, 14));
Type* const Type::Func14Type(new Type("func14", NULL, InstantiatedType::ObjectType, 15));
Type* const Type::Func15Type(new Type("func15", NULL, InstantiatedType::ObjectType, 16));
Type* const Type::Func16Type(new Type("func16", NULL, InstantiatedType::ObjectType, 17));
Type* const Type::Func17Type(new Type("func17", NULL, InstantiatedType::ObjectType, 18));
Type* const Type::Func18Type(new Type("func18", NULL, InstantiatedType::ObjectType, 19));
Type* const Type::Func19Type(new Type("func19", NULL, InstantiatedType::ObjectType, 20));

Type* const Type::StringType(new Type("string", NULL, InstantiatedType::ObjectType, 0));

Type* const Type::BoxedIntType(new Type("<Int>", NULL, InstantiatedType::ObjectType, 0));
Type* const Type::BoxedBoolType(new Type("<Bool>", NULL, InstantiatedType::ObjectType, 0));
Type* const Type::BoxedFloatType(new Type("<Float>", NULL, InstantiatedType::ObjectType, 0));

Type* const Type::RefType(new Type("<ref>", NULL, InstantiatedType::ObjectType, 1));

/** Cannot be named 'class', since 'class' is a keyword in the language */
Type* const Type::ClassType(new Type("classtype", NULL, InstantiatedType::ObjectType, 1));

/** moduletype should not be visible to the program */
Type* const Type::ModuleType(new Type("<moduletype>", NULL, InstantiatedType::ObjectType, 0));

Type* const Type::BoundlessType(new Type("boundless", NULL, NULL, 0));

Type* const Type::ListType(new Type("list", NULL, InstantiatedType::ObjectType, 1));
Type* const Type::MapType(new Type("map", NULL, InstantiatedType::ObjectType, 2));

static Type* const ftypes[] = {
  Type::Func0Type,
  Type::Func1Type,
  Type::Func2Type,
  Type::Func3Type,
  Type::Func4Type,
  Type::Func5Type,
  Type::Func6Type,
  Type::Func7Type,
  Type::Func8Type,
  Type::Func9Type,
  Type::Func10Type,
  Type::Func11Type,
  Type::Func12Type,
  Type::Func13Type,
  Type::Func14Type,
  Type::Func15Type,
  Type::Func16Type,
  Type::Func17Type,
  Type::Func18Type,
  Type::Func19Type,
};

const vector<Type*> Type::FuncTypes(ftypes, ftypes + VENOM_NELEMS(ftypes));

void Type::setClassSymbol(ClassSymbol* symbol) {
  assert(!this->symbol);
  assert(name == symbol->getName());
  assert(symbol->getType() == this);
  this->symbol = symbol;
}

void Type::ResetBuiltinTypes() {
  Type *types[] = {
    AnyType,
    IntType,
    BoolType,
    FloatType,
    StringType,
    VoidType,

    Func0Type,
    Func1Type,
    Func2Type,
    Func3Type,
    Func4Type,
    Func5Type,
    Func6Type,
    Func7Type,
    Func8Type,
    Func9Type,

    Func10Type,
    Func11Type,
    Func12Type,
    Func13Type,
    Func14Type,
    Func15Type,
    Func16Type,
    Func17Type,
    Func18Type,
    Func19Type,

    ObjectType,
    ClassType,
    ModuleType,

    BoxedIntType,
    BoxedBoolType,
    BoxedFloatType,

    RefType,

    ClassType,

    ModuleType,

    BoundlessType,

    ListType,
    MapType,
  };

  for (size_t i = 0; i < VENOM_NELEMS(types); i++) {
    types[i]->symbol = NULL;
  }
}

Type::~Type() {
  if (itype) delete itype;
}

SymbolTable*
Type::getClassSymbolTable() {
  return getClassSymbol()->getClassSymbolTable();
}

const SymbolTable*
Type::getClassSymbolTable() const {
  return getClassSymbol()->getClassSymbolTable();
}

string Type::stringify() const {
  stringstream buf;
  buf << getClassSymbolTable()->getSemanticContext()->getFullModuleName();
  buf << "." << name;
  return buf.str();
}

bool Type::isInt() const { return equals(*IntType); }
bool Type::isFloat() const { return equals(*FloatType); }
bool Type::isString() const { return equals(*StringType); }
bool Type::isBool() const { return equals(*BoolType); }
bool Type::isVoid() const { return equals(*VoidType); }
bool Type::isAny() const { return equals(*AnyType); }

bool Type::isListType() const { return equals(*ListType); }
bool Type::isMapType() const { return equals(*MapType); }
bool Type::isRefType() const { return equals(*RefType); }

bool Type::isFunction() const {
  for (vector<Type*>::const_iterator it = FuncTypes.begin();
       it != FuncTypes.end(); ++it) {
    if (equals(**it)) return true;
  }
  return false;
}

bool Type::isClassType() const { return equals(*Type::ClassType); }

bool Type::isModuleType() const {
  if (params) return false;
  // this is OK since isModuleType() never returns a non-const reference
  // to the caller (and itype acts more as a cache than as part of the
  // state of Type)
  Type *self = const_cast<Type*>(this);
  return self->instantiate()->isSubtypeOf(*InstantiatedType::ModuleType);
}

TypeParamType::TypeParamType(const string& name, size_t pos) :
    Type(name, NULL, InstantiatedType::AnyType, 0), pos(pos) {}

bool TypeParamType::equals(const Type& other) const {
  const TypeParamType* t =
    dynamic_cast<const TypeParamType*>(&other);
  if (!t) return false;
  if (!Type::equals(other)) return false;
  return pos == t->pos;
}

string TypeParamType::stringify() const {
  stringstream buf;
  buf << getName() << "$$" << util::stringify(pos + 1);
  return buf.str();
}

ASTExpressionNode*
Type::createDefaultInitializer() const {
  if (isInt()) return new IntLiteralNode(0);
  else if (isFloat()) return new DoubleLiteralNode(0.0);
  else if (isBool()) return new BoolLiteralNode(false);
  else return new NilLiteralNode;
}

InstantiatedType* Type::instantiate() {
  assert(params == 0);
  return itype ? itype : (itype = new InstantiatedType(this));
}

bool
InstantiatedType::isFullyInstantiated() const {
  VENOM_ASSERT_NOT_NULL(getType());
  if (dynamic_cast<const TypeParamType*>(getType())) return false;
  for (vector<InstantiatedType*>::const_iterator it = params.begin();
       it != params.end(); ++it) {
    if (!(*it)->isFullyInstantiated()) return false;
  }
  return true;
}

InstantiatedType* const InstantiatedType::IntType(Type::IntType->instantiate());
InstantiatedType* const InstantiatedType::BoolType(Type::BoolType->instantiate());
InstantiatedType* const InstantiatedType::FloatType(Type::FloatType->instantiate());
InstantiatedType* const InstantiatedType::StringType(Type::StringType->instantiate());
InstantiatedType* const InstantiatedType::VoidType(Type::VoidType->instantiate());
InstantiatedType* const InstantiatedType::BoxedIntType(Type::BoxedIntType->instantiate());
InstantiatedType* const InstantiatedType::BoxedBoolType(Type::BoxedBoolType->instantiate());
InstantiatedType* const InstantiatedType::BoxedFloatType(Type::BoxedFloatType->instantiate());
InstantiatedType* const InstantiatedType::ModuleType(Type::ModuleType->instantiate());
InstantiatedType* const InstantiatedType::BoundlessType(Type::BoundlessType->instantiate());

InstantiatedType*
Type::instantiate(SemanticContext* ctx) {
  return instantiate(ctx, vector<InstantiatedType*>());
}

InstantiatedType*
Type::instantiate(SemanticContext* ctx,
                  const vector<InstantiatedType*>& params) {
  if (this->params != params.size()) {
    throw invalid_argument(
        "Expected " + util::stringify(this->params) + " type params, got " +
        util::stringify(params.size()) + " type params");
  }
  if (this->params == 0) {
    return itype ? itype : (itype = new InstantiatedType(this));
  } else {
    return ctx->createInstantiatedType(this, params);
  }
}

InstantiatedType*
InstantiatedType::getParentInstantiatedType() {
  if (!getType()->getParent()) return NULL;
  TypeTranslator t;
  t.bind(this);
  return t.translate(getClassSymbolTable()->getSemanticContext(),
                     getType()->getParent());
}

struct equals_functor_t {
  inline bool operator()(const InstantiatedType* a,
                         const InstantiatedType* b) const {
    return !a->equals(*b);
  }
} equals_functor;

bool InstantiatedType::equals(const InstantiatedType& other) const {
  // equal if the types are equal and all params are equal
  if (!type->equals(*other.getType())) return false;
  assert(params.size() == other.getParams().size());
  InstantiatedTypeVec::const_iterator it =
    util::binpred_find_if(params.begin(), params.end(),
                          other.getParams().begin(), equals_functor);
  return it == params.end();
}

bool InstantiatedType::isSubtypeOf(const InstantiatedType& other) {
  InstantiatedType* cur = this;
  while (cur) {
    if (cur->equals(other)) return true;
    cur = cur->getParentInstantiatedType();
  }
  return false;
}

static void FillStack(stack<InstantiatedType*>& st, InstantiatedType* type) {
  InstantiatedType *cur = type;
  while (cur) {
    st.push(cur);
    cur = cur->getParentInstantiatedType();
  }
}

InstantiatedType*
InstantiatedType::mostCommonType(InstantiatedType* other) {
  // check for boundless type first
  if (getType()->isBoundlessType()) return other;
  if (other->getType()->isBoundlessType()) return this;

  // TODO: shortcut for when the two types are equal?

  stack<InstantiatedType*> a;
  stack<InstantiatedType*> b;
  FillStack(a, this);
  FillStack(b, other);

  assert(a.top()->equals(*b.top()));
  InstantiatedType *ret = NULL;
  while (!a.empty() && !b.empty() && a.top()->equals(*b.top())) {
    ret = a.top();
    a.pop();
    b.pop();
  }
  assert(ret);
  return ret;
}

string InstantiatedType::stringify() const {
  stringstream buf;
  buf << type->stringify();
  if (!params.empty()) {
    buf << "{";
    vector<string> s(params.size());
    transform(params.begin(), params.end(), s.begin(),
              util::stringify_functor<InstantiatedType>::ptr());
    buf << util::join(s.begin(), s.end(), ",");
    buf << "}";
  }
  return buf.str();
}

ClassSymbol*
InstantiatedType::findSpecializedClassSymbol() {
  InstantiatedType::AssertNoTypeParamPlaceholders(this);
  if (getParams().empty()) return getClassSymbol();
  SymbolTable* table = getClassSymbol()->getDefinedSymbolTable();
  TypeTranslator t;
  ClassSymbol* specialized = table->findClassSymbol(
      createClassName(), SymbolTable::NoRecurse, t);
  VENOM_ASSERT_NOT_NULL(specialized);
  return specialized;
}

string InstantiatedType::createClassNameImpl(bool fullName) const {
  stringstream buf;
  buf << (fullName ? type->stringify() : type->getName());
  if (!params.empty()) {
    buf << "{";
    vector<string> s(params.size());
    transform(params.begin(), params.end(), s.begin(),
              class_name_functor());
    buf << util::join(s.begin(), s.end(), ",");
    buf << "}";
  }
  return buf.str();
}

InstantiatedType*
InstantiatedType::refify(SemanticContext* ctx) {
  // should never refify a ref...
  assert(!getType()->equals(*Type::RefType));
  return Type::RefType->instantiate(ctx, util::vec1(this));
}

void
InstantiatedType::findMethodSymbolImpl(const string& name,
                                       MethodSymbol*& ms,
                                       InstantiatedType*& klass,
                                       bool findOrigDef) {
  // search cur symtab
  TypeTranslator t;
  FuncSymbol* fs =
    getClassSymbolTable()->findFuncSymbol(name, SymbolTable::NoRecurse, t);
  if (fs) {
    klass = this;
    VENOM_ASSERT_TYPEOF_PTR(MethodSymbol, fs);
    ms = static_cast<MethodSymbol*>(fs);
  }
  if (findOrigDef || !ms) {
    InstantiatedType* ptype = getParentInstantiatedType();
    if (ptype) ptype->findMethodSymbolImpl(name, ms, klass, true);
  }
}

}
}
