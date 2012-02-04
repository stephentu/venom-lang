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

#include <algorithm>
#include <cassert>
#include <utility>

#include <analysis/boundfunction.h>
#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>
#include <analysis/typetranslator.h>

#include <util/stl.h>

using namespace std;

namespace venom {
namespace analysis {

InstantiatedType*
TypeTranslator::translate(SemanticContext* ctx, InstantiatedType* type) const {
  bool changed = false;
  InstantiatedType* ret = translateImpl(ctx, type, changed);
  while (changed) {
    changed = false;
    ret = translateImpl(ctx, ret, changed);
  }
  return ret;
}

void
TypeTranslator::translate(
      SemanticContext* ctx,
      BoundFunction& from,
      BoundFunction& result) const {
  // create a new dummy type, and then use itype's translator
  string dummyName = "dummy$$" + util::stringify(ctx->uniqueId());
  Type* dummyType =
    ctx->createType(dummyName,
                    InstantiatedType::AnyType,
                    from.first->getTypeParams().size());
  InstantiatedType* dummyItype =
    dummyType->instantiate(ctx, from.first->getTypeParams());
  InstantiatedType* translated = translate(ctx, dummyItype);
  result.first = from.first;
  result.second = translated->getParams();
}

struct find_functor {
  find_functor(InstantiatedType* type) : type(type) {}
  inline bool operator()(const InstantiatedTypePair& p) const {
    return p.first->equals(*type);
  }
  InstantiatedType* type;
};

InstantiatedType*
TypeTranslator::translateImpl(
    SemanticContext* ctx, InstantiatedType* type, bool& changed) const {
  TypeMap::const_iterator it =
    find_if(map.begin(), map.end(), find_functor(type));
  if (it != map.end()) {
    changed = true;
    return it->second;
  }
  vector<InstantiatedType*> buf(type->getParams().size());
  transform(type->getParams().begin(), type->getParams().end(),
            buf.begin(), TranslateImplFunctor(ctx, this, &changed));
  return changed ? ctx->createInstantiatedType(type->getType(), buf) : type;
}

void
TypeTranslator::bind(InstantiatedType* type) {
  vector<InstantiatedType*> &lhs =
    type->getType()->getClassSymbol()->getTypeParams();
  vector<InstantiatedType*> &rhs = type->getParams();
  bind(lhs, rhs);
}

void
TypeTranslator::bind(BoundFunction& func) {
  vector<InstantiatedType*>& lhs = func.first->getTypeParams();
  vector<InstantiatedType*>& rhs = func.second;
  bind(lhs, rhs);
}

void
TypeTranslator::bind(const vector<InstantiatedType*>& lhs,
                     const vector<InstantiatedType*>& rhs) {
  assert(lhs.size() == rhs.size());
  TypeMap tmap;
  tmap.reserve(lhs.size());
  for (size_t i = 0; i < lhs.size(); i++) {
    if (!lhs[i]->equals(*rhs[i])) {
      tmap.push_back(make_pair(lhs[i], rhs[i]));
    }
  }
  map.insert(map.end(), tmap.begin(), tmap.end());
}

void
TypeTranslator::addTranslation(InstantiatedType* from,
                               InstantiatedType* to) {
  map.push_back(make_pair(from, to));
}

void
TypeTranslator::printStderr() const {
  cerr << "{";

  for (TypeMap::const_iterator it = map.begin();
       it != map.end(); ++it) {
    cerr << it->first->stringify() << " => " << it->second->stringify();
    if ((it + 1) != map.end()) {
      cerr << "," << endl;
    }
  }

  cerr << "}";
}

}
}
