// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
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

//! Assertion with message but don't abort afterwards.
#define NERVE_WASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg__ << std::endl;\
    }\
  } while(false);
