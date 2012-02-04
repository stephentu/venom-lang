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

#include <runtime/box.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

venom_class_object& venom_integer::IntegerClassTable() {
  static venom_class_object c(
    "<Int>",
    sizeof(venom_integer),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

venom_class_object& venom_double::DoubleClassTable() {
  static venom_class_object c(
    "<Float>",
    sizeof(venom_double),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

venom_class_object& venom_boolean::BooleanClassTable() {
  static venom_class_object c(
    "<Bool>",
    sizeof(venom_boolean),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

}
}
