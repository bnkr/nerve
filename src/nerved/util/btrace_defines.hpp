// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef UTIL_BTRACE_DEFINES_HPP_ttjj3nzz
#define UTIL_BTRACE_DEFINES_HPP_ttjj3nzz

#include "../defines.hpp"

#ifndef NERVE_DEVELOPER
#  error "NERVE_DEVELOPER must be defined"
#endif

#if ! NERVE_DEVELOPER
// Get rid of non-fixed asserts
#  define BTRACE_DISABLE_ASSERT
// Just get file/lineinfo.
#  define BTRACE_DOC_LOCATION
#endif

#endif
