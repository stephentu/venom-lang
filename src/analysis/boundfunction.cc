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

#include <cassert>
#include <sstream>

#include <analysis/boundfunction.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <util/stl.h>

using namespace std;

namespace venom {
namespace analysis {

BoundFunction::BoundFunction(FuncSymbol* symbol,
                             const vector<InstantiatedType*>& params)
    : SymTypeParamsPair(symbol, params) {
  assert(symbol->getTypeParams().size() == params.size());
}

bool
BoundFunction::isFullyInstantiated() const {
  return InstantiatedType::IsFullyInstantiated(second.begin(), second.end());
}

bool
BoundFunction::isCodeGeneratable() const {
  return second.empty();
}

string
BoundFunction::createFuncName() const {
  stringstream buf;
  buf << first->getName();
  if (!second.empty()) {
    buf << "{";
    vector<string> names;
    names.reserve(second.size());
    for (vector<InstantiatedType*>::const_iterator it = second.begin();
         it != second.end(); ++it) {
      names.push_back((*it)->stringify());
    }
    buf << util::join(names.begin(), names.end(), ",");
    buf << "}";
  }
  return buf.str();
}

FuncSymbol*
BoundFunction::findSpecializedFuncSymbol() {
  InstantiatedType::AssertNoTypeParamPlaceholders(second);
  if (second.empty()) return first;
  SymbolTable* table = first->getDefinedSymbolTable();
  TypeTranslator t;
  FuncSymbol* specialized = table->findFuncSymbol(
      createFuncName(), SymbolTable::NoRecurse, t);
  VENOM_ASSERT_NOT_NULL(specialized);
  return specialized;
}

}
}
