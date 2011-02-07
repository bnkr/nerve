// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef ASSERTS_HPP_45mln30x
#define ASSERTS_HPP_45mln30x
#include <iostream>
#include <cstdlib>

//! Abort with a sensible message.
#define NERVE_ABORT(msg__)\
  do {\
    std::cerr << "nerve: aborting: " << msg__ << std::endl;\
    std::abort();\
  } while (false);


//! Assertion with message.
#define NERVE_ASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg__ << std::endl;\
      std::abort();\
    }\
  } while(false);

//! Alias.
#define NERVE_ASSERT_PTR(var__) NERVE_ASSERT(var__ != NULL, #var__ " must not be null")

namespace asserts {
  namespace detail {
    template<class T> T *check_pointer(T *p, const char *code) {
      NERVE_ASSERT(p != NULL, "pointer in " << code << " must not be null");
      return p;
    }
  }
}

//! Check the pointer is non-null and return it.
#define NERVE_CHECK_PTR(expr__) ::asserts::detail::check_pointer((expr__), #expr__)

//! Assertion with message but don't abort afterwards.
#define NERVE_WASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg__ << std::endl;\
    }\
  } while(false);

#endif
