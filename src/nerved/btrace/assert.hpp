// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
/*!
 * \file
 * \ingroup grp_asserts
 *
 * Assertion macros.  This header can be included multiple times.
 */

#include "detail/asserts.hpp"

#undef BTRACE_ASSERT
#undef BTRACE_WASSERT
#undef BTRACE_ASSERT_PTR
#undef BTRACE_WASSERT_PTR
#undef BTRACE_FIXED_ASSERT
#undef BTRACE_FIXED_ASSERT_PTR
#undef BTRACE_DETAIL_DOCS
#undef BTRACE_DETAIL_F

#  if defined(BTRACE_DISABLE_DOCS) && defined(BTRACE_DISABLE_STRINGS)
#    define BTRACE_DETAIL_DOCS(expr__, ...)
#  elif defined(BTRACE_DISABLE_DOCS)
#    define BTRACE_DETAIL_DOCS(expr__, ...) , __FILE__, __LINE__, ""
#  else
#    define BTRACE_DETAIL_DOCS(expr__, ...) , __FILE__, __LINE__, __VA_ARGS__
#  endif

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
#  define BTRACE_ASSERT(expr__, ...) \
  ::btrace::detail::BTRACE_DETAIL_F ## _true((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));

//! \ingroup grp_asserts
//! Assertion which isn't conditional on DISABLE macros.
#  define BTRACE_FIXED_ASSERT(expr__, ...) \
  ::btrace::detail::assert_true((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));

#  if defined(BTRACE_DISABLE_DOCS) && defined(BTRACE_DISABLE_STRINGS)
#    define BTRACE_WASSERT(expr__, ...)
#  else
//! \ingroup grp_asserts
//! Assert but don't exit on a failure.
#      define BTRACE_WASSERT(expr__, ...) \
  ::btrace::detail::wassert_true((expr__), BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));
#  endif

//! \ingroup grp_asserts
//! Assert that expr__ yields a non-null pointer and returns that pointer.
#  define BTRACE_ASSERT_PTR(expr__, ...) \
  ::btrace::detail::BTRACE_DETAIL_F ## _nn((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));

#  if defined(BTRACE_DISABLE_DOCS) && defined(BTRACE_DISABLE_STRINGS)
#    define BTRACE_WASSERT_PTR(expr__, ...)
#  else
//! \ingroup grp_asserts
//! Assert but don't exit on a failure.
#      define BTRACE_WASSERT_PTR(expr__, ...) \
  ::btrace::detail::wassert_nn((expr__), BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));
#  endif
#endif /* BTRACE_ASSERT_DISABLE */

//! \ingroup grp_asserts
//! As BTRACE_ASSERT but not conditional on a disable macro.
#define BTRACE_FIXED_ASSERT(expr, ...)\
  ::btrace::detail::assert_true((expr__), #expr__, BTRACE_DETAIL_DOCS(__VA_ARGS__));

//! \ingroup grp_asserts
//! As BTRACE_ASSERT_PTR but not conditional on a disable macro.
#  define BTRACE_FIXED_ASSERT_PTR(expr__, ...) \
  ::btrace::detail::assert_nn((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));
