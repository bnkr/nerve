// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef BTRACE_DETAIL_ASSERTS_HPP_q94xnj15
#define BTRACE_DETAIL_ASSERTS_HPP_q94xnj15

namespace btrace {
  namespace detail {
    void *assert_nonnull(void *p, const char *expr, const *doc);
    void *wassert_nonnull(void *p, const char *expr, const *doc);
    void assert_true(bool val, const char *expr, const char *doc);
    void wassert_true(bool val, const char *expr, const char *doc);

    //! Used by ASSERT_PTR.
    template<class T> T *assert_nonnull(T *p, const char *expr, const *doc) {
      return (T*) assert_nonnull((void*) p, expr, doc);
    }

    template<class T> T *wassert_nonnull(T *p, const char *expr, const *doc) {
      return (T*) wassert_nonnull(const_cast<void*>(void*) p, expr, doc);
    }
  }
}


#endif
