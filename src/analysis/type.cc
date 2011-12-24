#include <analysis/semanticcontext.h>
#include <analysis/type.h>

using namespace std;

namespace venom {
namespace analysis {

Type* Type::AnyType(new Type("any", NULL, NULL, 0));
Type* Type::IntType(new Type("int", NULL, AnyType, 0));
Type* Type::BoolType(new Type("bool", NULL, AnyType, 0));
Type* Type::FloatType(new Type("float", NULL, AnyType, 0));
Type* Type::StringType(new Type("string", NULL, AnyType, 0));
Type* Type::VoidType(new Type("void", NULL, AnyType, 0));
Type* Type::ObjectType(new Type("object", NULL, AnyType, 0));

Type* Type::ListType(new Type("list", NULL, ObjectType, 1));
Type* Type::MapType(new Type("map", NULL, ObjectType, 2));

Type::~Type() {
  if (itype) delete itype;
}

InstantiatedType* Type::instantiate() {
  assert(params == 0);
  return itype ? itype : (itype = new InstantiatedType(this));
}

InstantiatedType* InstantiatedType::AnyType(Type::AnyType->instantiate());
InstantiatedType* InstantiatedType::IntType(Type::IntType->instantiate());
InstantiatedType* InstantiatedType::BoolType(Type::BoolType->instantiate());
InstantiatedType* InstantiatedType::FloatType(Type::FloatType->instantiate());
InstantiatedType* InstantiatedType::StringType(Type::StringType->instantiate());
InstantiatedType* InstantiatedType::VoidType(Type::VoidType->instantiate());
InstantiatedType* InstantiatedType::ObjectType(Type::ObjectType->instantiate());

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

}
}
