// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "demangle.hpp"

#include <cxxabi.h>
#include <cassert>
#include <cstring>
#include <cstdlib>

//! This memory is on the heap and must be freed
char *btrace::detail::cxxabi_demangle(const char *name) {
  assert(name != NULL);
  int status;
  char *ret = ::abi::__cxa_demangle(name, NULL, NULL, &status);
  switch (status) {
  case 0:
    break;
  case -1:
    throw std::bad_alloc();
  case -2: // not a mangled name; it's already demangled or it's a C name.
    {
      const std::size_t name_length = std::strlen(name);
      ret = (char *) std::malloc(name_length + 1);
      if (! ret) throw std::bad_alloc();
      std::strcpy(ret, name);
    }
    break;
  case -3: // invalid argument
    ret = NULL;
    break;
  }

  return ret;
}

namespace {
  template<class T>
  class malloc_ptr {
    public:
    malloc_ptr(T *p) : p_(p) {}
    ~malloc_ptr() { std::free(p_); }

    T *get() const { return p_; }

    private:
    T *p_;
  };
}

std::string btrace::demangle_name(const char *m) {
  malloc_ptr<char> p = detail::cxxabi_demangle(m);
  if (p.get() == NULL) {
    std::string s(m);
    s += " <not demangleable>";
    return s;
  }
  else {
    return std::string(p.get());
  }
}
