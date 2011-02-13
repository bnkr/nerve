// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef CONFIG_C_STRING_HPP_i8ip7xsm
#define CONFIG_C_STRING_HPP_i8ip7xsm
#include "../util/asserts.hpp"

#include <cstring>

//! Something we can put in a map.
struct c_string {
  explicit c_string(const char *c) : str_(NERVE_CHECK_PTR(c)) {}
  bool operator<(const c_string &c) const { return std::strcmp(c.str_, this->str_) < 0; }
  const char *c_str() { return str_; }
  private:
  const char *str_;
};

#endif
