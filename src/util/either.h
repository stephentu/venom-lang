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

#ifndef VENOM_UTIL_EITHER_H
#define VENOM_UTIL_EITHER_H

#include <cassert>

namespace venom {
namespace util {

/**
 * Both Left and Right are required to have default
 * no arg ctors. Left and Right must be different types. That is,
 * it must be valid C++ to define two constructors (one of type Left
 * and one of type Right)
 */
template <typename Left, typename Right>
class _either {
public:
  /** Left constructor */
  _either(const Left& leftItem) :
    _left(true), leftItem(leftItem) {}
  /** Right constructor */
  _either(const Right& rightItem) :
    _left(false), rightItem(rightItem) {}

  inline bool isLeft() const { return _left; }
  inline bool isRight() const { return !_left; }

  inline Left& left() { assert(isLeft()); return leftItem; }
  inline const Left& left() const { assert(isLeft()); return leftItem; }

  inline Right& right() { assert(isRight()); return rightItem; }
  inline const Right& right() const { assert(isRight()); return rightItem; }

protected:
  bool _left;
  Left leftItem;
  Right rightItem;
};

/**
 * Both left/right are required to have
 * operator < and operator ==
 */
template <typename Left, typename Right>
class _either_comparable : public _either<Left, Right> {
public:
  _either_comparable(const Left& leftItem) :
    _either<Left, Right>(leftItem) {}
  _either_comparable(const Right& rightItem) :
    _either<Left, Right>(rightItem) {}

  inline bool operator<(const _either_comparable<Left, Right>& b) const {
    if (this->isLeft()) {
      // left always ahead of right
      return b.isLeft() ? this->left() < b.left() : true;
    } else {
      return b.isLeft() ? false : this->right() < b.right();
    }
  }

  inline bool operator==(const _either_comparable<Left, Right>& b) const {
    if (this->isLeft()) {
      return b.isLeft() ? this->left() == b.left() : false;
    } else {
      return b.isLeft() ? false : this->right() == b.right();
    }
  }
};

template <typename Left, typename Right>
struct either {
  typedef _either<Left, Right> non_comparable;
  typedef _either_comparable<Left, Right> comparable;
};

}
}

#endif /* VENOM_UTIL_EITHER_H */
