#ifndef VENOM_UTIL_NONCOPYABLE_H
#define VENOM_UTIL_NONCOPYABLE_H

// Taken from the Boost library
// http://www.boost.org/doc/libs/1_48_0/boost/noncopyable.hpp

namespace venom {
namespace util {

//  Private copy constructor and copy assignment ensure classes derived from
//  class noncopyable cannot be copied.

//  Contributed by Dave Abrahams

namespace noncopyable_  // protection from unintended ADL
{
  class noncopyable
  {
   protected:
      noncopyable() {}
      ~noncopyable() {}
   private:  // emphasize the following members are private
      noncopyable( const noncopyable& );
      const noncopyable& operator=( const noncopyable& );
  };
}

typedef noncopyable_::noncopyable noncopyable;

}
}

#endif /* VENOM_UTIL_NONCOPYABLE_H */
