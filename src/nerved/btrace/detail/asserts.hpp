// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef BTRACE_DETAIL_ASSERTS_HPP_q94xnj15
#define BTRACE_DETAIL_ASSERTS_HPP_q94xnj15

namespace btrace {
  namespace detail {
    typedef const char *cstr_t;

    // This should help reduce the binary size as the unused functions can be
    // dropped.  Ditto alsways passing the doc string -- we have to test it for
    // emptyness anyway so may as well use the same symbol.

    void assert_true(bool);
    void assert_true(bool, cstr_t file, int line);
    void assert_true(bool, cstr_t file, int line, cstr_t expr, cstr_t doc);

    void *assert_nn_static(void*);
    void *assert_nn_static(void*, cstr_t file, int line);
    void *assert_nn_static(void*, cstr_t file, int line, cstr_t expr, cstr_t doc);

    template<class T>
    T *assert_nn(T *p) {
      return (T*) assert_nn_static((void*) p);
    }

    template<class T>
    T *assert_nn(T *p, cstr_t file, int line) {
      return (T*) assert_nn_static((void*) p, file, line);
    }

    template<class T>
    T *assert_nn(T *p, cstr_t file, int line, cstr_t expr, cstr_t doc) {
      return (T*) assert_nn_static((void*) p, file, line, expr, doc);
    }
  }
}


#endif
