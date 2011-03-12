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
#undef BTRACE_DETAIL_ASSERT_FUNC
#undef BTRACE_DETAIL_ASSERT_PTR_FUNC

#if defined(BTRACE_DOC_NONE) && defined(BTRACE_DOC_LOCATION)
#  error "BTRACE_DOC_NONE and BTRACE_DOC_LOCATION can't be defined together."
#elif defined(BTRACE_DOC_NONE) && defined(BTRACE_DOC_CODE)
#  error "BTRACE_DOC_NONE and BTRACE_DOC_CODE can't be defined together."
#elif defined(BTRACE_DOC_CODE) && defined(BTRACE_DOC_NONE)
#  error "BTRACE_DOC_CODE and BTRACE_DOC_NONE can't be defined together."
#endif

#  if defined(BTRACE_DOC_NONE)
#    define BTRACE_DETAIL_DOCS(expr__, ...)
#  elif defined(BTRACE_DOC_LOCATION)
#    define BTRACE_DETAIL_DOCS(expr__, ...) , __FILE__, __LINE__
#  elif defined(BTRACE_DOC_CODE)
#    define BTRACE_DETAIL_DOCS(expr__, ...) , __FILE__, __LINE__, #expr__, ""
#  else
#    define BTRACE_DETAIL_DOCS(expr__, ...) , __FILE__, __LINE__, #expr__, __VA_ARGS__
#  endif

#if defined(BTRACE_ASSERT_DISABLE) && defined(BTRACE_ASSERT_WARN)
#  error "BTRACE_ASSERT_DISABLE and BTRACE_ASSERT_WARN may not both be defined"
#elif defined(BTRACE_ASSERT_DISABLE)
#  define BTRACE_ASSERT(expr__, ...)
#  define BTRACE_ASSERT_PTR(expr__, ...) expr__
#  define BTRACE_WASSERT(expr__, ...)
#  define BTRACE_WASSERT_PTR(expr__, ...) expr__
#else

#  ifdef BTRACE_ASSERT_WARN
#    define BTRACE_DETAIL_ASSERT_FUNC wassert_true
#    define BTRACE_DETAIL_ASSERT_PTR_FUNC wassert_nn
#  else
#    define BTRACE_DETAIL_ASSERT_FUNC assert_true
#    define BTRACE_DETAIL_ASSERT_PTR_FUNC assert_nn
#  endif

//! \ingroup grp_asserts
//! Assert that expr__ yields true.
#  define BTRACE_ASSERT(expr__, ...) \
  ::btrace::detail::BTRACE_DETAIL_ASSERT_FUNC((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));

//! \ingroup grp_asserts
//! Assert that expr__ yields a non-null pointer and returns that pointer.
#  define BTRACE_ASSERT_PTR(expr__, ...) \
  (::btrace::detail::BTRACE_DETAIL_ASSERT_PTR_FUNC((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__)))

#  if defined(BTRACE_DISABLE_DOCS) && defined(BTRACE_DISABLE_STRINGS)
#    define BTRACE_WASSERT(expr__, ...)
#    define BTRACE_WASSERT_PTR(expr__, ...)
#  else
//! \ingroup grp_asserts
//! Assert but don't exit on a failure.
#      define BTRACE_WASSERT(expr__, ...) \
  ::btrace::detail::wassert_true((expr__), BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__));
//! \ingroup grp_asserts
//! Assert but don't exit on a failure.
#      define BTRACE_WASSERT_PTR(expr__, ...) \
  (::btrace::detail::wassert_nn((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__)))
#  endif
#endif /* BTRACE_ASSERT_DISABLE */

//! \ingroup grp_asserts
//! As BTRACE_ASSERT but not conditional on a disable macro.
#define BTRACE_FIXED_ASSERT(expr__, ...)\
  ::btrace::detail::assert_true((expr__) BTRACE_DETAIL_DOCS(exper__, "" __VA_ARGS__));

//! \ingroup grp_asserts
//! As BTRACE_ASSERT_PTR but not conditional on a disable macro.
#  define BTRACE_FIXED_ASSERT_PTR(expr__, ...) \
  (::btrace::detail::assert_nn((expr__) BTRACE_DETAIL_DOCS(expr__, "" __VA_ARGS__)))
