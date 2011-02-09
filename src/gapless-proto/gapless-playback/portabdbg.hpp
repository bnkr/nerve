// pretend to be bdbg!

#include <iostream>

#define trc(m__) std::cout << m__ << std::endl;

#define wmassert(code__, msg__)\
  do {\
    if (! (code__)) {\
      std::cerr << "assert: " << #code__ << ": " << msg__ << std::endl;\
    }\
  }\
  while (false);

#define wmassert_eq(lhs__, rhs__, msg__) wmassert((lhs__) == (rhs__), msg__);

