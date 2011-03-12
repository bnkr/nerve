// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
/*!
 * \file
 * \ingroup grp_asserts
 *
 * Assertion macros.  This header can be included multiple times.
 *
 * Use BTRACE_NDEBUG to disable assertions.
 */

#include "detail/asserts.hpp"

#undef BTRACE_ASSERT
#undef BTRACE_WASSERT
#undef BTRACE_ASSERT_PTR

#if defined(BTRACE_ASSERT_DISABLE) && defined(BTRACE_ASSERT_WARN)
#  error "BTRACE_ASSERT_DISABLE and BTRACE_ASSERT_WARN may not both be defined"
#elif defined(BTRACE_ASSERT_DISABLE)
#  define BTRACE_ASSERT(expr__, ...)
#  define BTRACE_ASSERT_PTR(expr__, ...)
#  define BTRACE_WASSERT(expr__, ...)
#else
#  ifdef BTRACE_ASSERT_WARN
#    define BTRACE_DETAIL_F wassert
#  else
#    define BTRACE_DETAIL_F assert
#  endif

//! \ingroup grp_asserts
//! Assert that expr__ yields true.
#  define BTRACE_ASSERT(expr__, ...) ::btrace::detail::BTRACE_DETAIL_F ## _true((expr__), #expr__, "" __VA_ARGS__);

//! \ingroup grp_asserts
//! Assert that expr__ yields a non-null pointer and returns that pointer.
#  define BTRACE_ASSERT_PTR(expr__, ...) ::btrace::detail::BTRACE_DETAIL_F ## _nonull((expr__), #expr__, "" __VA_ARGS__)

//! \ingroup grp_asserts
//! Assert but don't exit.
#  define BTRACE_WASSERT(expr__, ...) ::btrace::detail::wassert_nonull((expr__), #expr__, "" __VA_ARGS__)

#undef BTRACE_DETAIL_F

#endif
