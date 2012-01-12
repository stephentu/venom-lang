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
