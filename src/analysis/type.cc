#include <algorithm>
#include <sstream>
#include <stack>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/type.h>

#include <util/stl.h>

using namespace std;

namespace venom {
namespace analysis {

Type* Type::AnyType(new Type("any", NULL, NULL, 0));
InstantiatedType* InstantiatedType::AnyType(Type::AnyType->instantiate());

Type* Type::IntType(new Type("int", NULL, InstantiatedType::AnyType, 0));
Type* Type::BoolType(new Type("bool", NULL, InstantiatedType::AnyType, 0));
Type* Type::FloatType(new Type("float", NULL, InstantiatedType::AnyType, 0));
Type* Type::StringType(new Type("string", NULL, InstantiatedType::AnyType, 0));
Type* Type::VoidType(new Type("void", NULL, InstantiatedType::AnyType, 0));

Type* Type::Func0Type(new Type("func0", NULL, InstantiatedType::AnyType, 1));
Type* Type::Func1Type(new Type("func1", NULL, InstantiatedType::AnyType, 2));
Type* Type::Func2Type(new Type("func2", NULL, InstantiatedType::AnyType, 3));
Type* Type::Func3Type(new Type("func3", NULL, InstantiatedType::AnyType, 4));
Type* Type::Func4Type(new Type("func4", NULL, InstantiatedType::AnyType, 5));
Type* Type::Func5Type(new Type("func5", NULL, InstantiatedType::AnyType, 6));
Type* Type::Func6Type(new Type("func6", NULL, InstantiatedType::AnyType, 7));
Type* Type::Func7Type(new Type("func7", NULL, InstantiatedType::AnyType, 8));
Type* Type::Func8Type(new Type("func8", NULL, InstantiatedType::AnyType, 9));
Type* Type::Func9Type(new Type("func9", NULL, InstantiatedType::AnyType, 10));

Type* Type::Func10Type(new Type("func10", NULL, InstantiatedType::AnyType, 11));
Type* Type::Func11Type(new Type("func11", NULL, InstantiatedType::AnyType, 12));
Type* Type::Func12Type(new Type("func12", NULL, InstantiatedType::AnyType, 13));
Type* Type::Func13Type(new Type("func13", NULL, InstantiatedType::AnyType, 14));
Type* Type::Func14Type(new Type("func14", NULL, InstantiatedType::AnyType, 15));
Type* Type::Func15Type(new Type("func15", NULL, InstantiatedType::AnyType, 16));
Type* Type::Func16Type(new Type("func16", NULL, InstantiatedType::AnyType, 17));
Type* Type::Func17Type(new Type("func17", NULL, InstantiatedType::AnyType, 18));
Type* Type::Func18Type(new Type("func18", NULL, InstantiatedType::AnyType, 19));
Type* Type::Func19Type(new Type("func19", NULL, InstantiatedType::AnyType, 20));

Type* Type::ObjectType(new Type("object", NULL, InstantiatedType::AnyType, 0));
InstantiatedType* InstantiatedType::ObjectType(Type::ObjectType->instantiate());

Type* Type::BoundlessType(new Type("boundless", NULL, NULL, 0));

Type* Type::ListType(new Type("list", NULL, InstantiatedType::ObjectType, 1));
Type* Type::MapType(new Type("map", NULL, InstantiatedType::ObjectType, 2));

void Type::setClassSymbol(ClassSymbol* symbol) {
  assert(!this->symbol);
  assert(name == symbol->getName());
  assert(symbol->getType() == this);
  this->symbol = symbol;
}

Type::~Type() {
  if (itype) delete itype;
}

bool Type::isNumeric() const {
  return equals(*IntType) || equals(*FloatType);
}

InstantiatedType* Type::instantiate() {
  assert(params == 0);
  return itype ? itype : (itype = new InstantiatedType(this));
}

InstantiatedType* InstantiatedType::IntType(Type::IntType->instantiate());
InstantiatedType* InstantiatedType::BoolType(Type::BoolType->instantiate());
InstantiatedType* InstantiatedType::FloatType(Type::FloatType->instantiate());
InstantiatedType* InstantiatedType::StringType(Type::StringType->instantiate());
InstantiatedType* InstantiatedType::VoidType(Type::VoidType->instantiate());
InstantiatedType* InstantiatedType::BoundlessType(Type::BoundlessType->instantiate());

InstantiatedType*
Type::instantiate(SemanticContext* ctx) {
  return instantiate(ctx, vector<InstantiatedType*>());
}

InstantiatedType*
Type::instantiate(SemanticContext* ctx,
                  const vector<InstantiatedType*>& params) {
  if (this->params != params.size()) return NULL;
  if (this->params == 0) {
    return itype ? itype : (itype = new InstantiatedType(this));
  } else {
    return ctx->createInstantiatedType(this, params);
  }
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

bool InstantiatedType::isSubtypeOf(const InstantiatedType& other) const {
  const InstantiatedType* cur = this;
  while (cur) {
    if (cur->equals(other)) return true;
    cur = cur->getType()->getParent();
  }
  return false;
}

static void FillStack(stack<InstantiatedType*>& st, InstantiatedType* type) {
  InstantiatedType *cur = type;
  while (cur) {
    st.push(cur);
    cur = cur->getType()->getParent();
  }
}


InstantiatedType*
InstantiatedType::mostCommonType(InstantiatedType* other) {
  // check for boundless type first
  if (getType()->isBoundlessType()) return other;
  if (other->getType()->isBoundlessType()) return this;

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

struct stringify_functor_t {
  inline string operator()(const InstantiatedType* t) const {
    return t->stringify();
  }
} stringify_functor;

string InstantiatedType::stringify() const {
  stringstream buf;
  buf << type->getName();
  if (!params.empty()) {
    buf << "<";
    vector<string> s(params.size());
    transform(params.begin(), params.end(), s.begin(), stringify_functor);
    buf << util::join(s.begin(), s.end(), ",");
    buf << ">";
  }
  return buf.str();
}

}
}
