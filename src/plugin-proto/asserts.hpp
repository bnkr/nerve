#include <iostream>
#include <cstdlib>

//! Abort with a sensible message.
#define NERVE_ABORT(msg__)\
  do {\
    std::cerr << "nerve: eeek: " << msg___ << std::endl;\
    std::abort();\
  } while (false);


//! Assertion with message.
#define NERVE_ASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg___ << std::endl;\
      std::abort();\
    }\
  } while(false);

//! Assertion with message but don't abort afterwards.
#define NERVE_WASSERT(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "nerve: assert: '" << #code__ << "': " << msg___ << std::endl;\
    }\
  } while(false);
