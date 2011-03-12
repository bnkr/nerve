// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef UTIL_ASSERTS_HPP_45mln30x
#define UTIL_ASSERTS_HPP_45mln30x

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "btrace_defines.hpp"
#include "../btrace/assert.hpp"

namespace asserts {
  namespace detail {
    template<class T> T *check_pointer(int line, const char *file, T *p, const char *code) {
      if (p == NULL) {
        std::cerr
          << "nerve: " << file << ":" << line << ":\n"
          << "  expression: '" << code << "'\n"
          << "  must yield a non-null pointer" << std::endl;
        std::abort();
      }
      return p;
    }
  }
}

//! \ingroup grp_asserts
//! Abort with a sensible message.
#define NERVE_ABORT(msg__)\
  do {\
    std::cerr << "nerve: aborting: " << msg__ << std::endl;\
    std::abort();\
  } while (false);


//! \ingroup grp_asserts
//! Assertion with message.
#define NERVE_ASSERT(code__, msg__) BTRACE_ASSERT(code__, msg__)

//! \ingroup grp_asserts
//! Alias.
#define NERVE_ASSERT_PTR(var__) NERVE_ASSERT(var__ != NULL, #var__ " must not be null")

//! \ingroup grp_asserts
//! Check the pointer is non-null and return it.
#define NERVE_CHECK_PTR(expr__) BTRACE_ASSERT_PTR(expr__)

//! \ingroup grp_asserts
//! Assertion with message but don't abort afterwards.
#define NERVE_WASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg__ << std::endl;\
    }\
  } while(false);

//! \ingroup grp_asserts
//! Something is not implemented
#define NERVE_NIMPL(what__)\
  do {\
    std::cerr << __FILE__ << ":" << __LINE__ << ": warning: " << std::endl;\
    std::cerr << "  in " << __PRETTY_FUNCTION__ << std::endl;\
    std::cerr << "  not implemented: " << what__ << std::endl;\
  } while (false);

//! \ingroup grp_asserts
//! Wipe memory.
#define NERVE_WIPE(p__, size__) std::memset(p__, 0, size__)

#endif
