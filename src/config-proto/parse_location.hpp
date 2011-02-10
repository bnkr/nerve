// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef CONFIG_PARSE_LOCATION_HPP_qtoy4u4y
#define CONFIG_PARSE_LOCATION_HPP_qtoy4u4y

#include <cstdlib>

namespace config {
  /*!
   * \ingroup grp_config
   * Locational state handled by the lexer (and whatever bit knows about he
   * filename).
   */
  class parse_location {
    public:

    parse_location() { new_file(NULL); }

    int line() const { return line_; }
    const char *file() const { return file_; }

    void increment() { ++line_; }
    void new_file(const char *file) { file_ = file; line_ = 1; }

    private:

    // TODO:
    //   char* only works by fluke because we use CLI arguments.

    const char *file_;
    int line_;
  };
} // ns config

#endif
