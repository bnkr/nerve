// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * The config file parser.  This combines the flex and lemon bits into stuff we
 * actually want to use.
 */

#ifndef CONFIG_PARSER_HPP_999mx4dk
#define CONFIG_PARSER_HPP_999mx4dk

namespace config {
  class pipeline_config;

  //! Main interface to configuration parsing.
  class config_parser {
    public:

    struct params {
      const char *file() const;
      params &file(const char *file);
      bool trace_general() const;
      bool trace_parser() const;
      bool trace_lexer() const;
      bool lexer_only() const;
    };

    config_parser(const params &p) : p_(p) {}

    void parse(pipeline_config &output);

    private:

    params p_;
  };
}

#include "lemon_interface.hpp"
#include "flex_interface.hpp"
#include "pipeline_configs.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>

struct stdio_ptr {
  stdio_ptr() : fh_(NULL) {}
  stdio_ptr(FILE *fh) : fh_(fh) {}

  ~stdio_ptr() { close(); }

  void reset(FILE *fh) { close(); fh_ = fh; }
  FILE *get() { return fh_; }

  void close() {
    if (fh_) std::fclose(fh_);
  }

  private:
  FILE *fh_;
};

void config::config_parser::parse(config::pipeline_config &output) {
  stdio_ptr fh;
  if (std::strcmp(p_.file(), "-") != 0) {
    fh.reset(std::fopen(p_.file(), "r"));
    if (! fh.get() || std::ferror(fh.get())) {
      // TODO: exceptions?
      std::cerr << "xxsc-compile: could not open file '" << p_.file() << "'" << std::endl;
      return;
    }
  }

  // TODO:
  //   Perhaps the class is better named "lemon interface" to be clear it's not
  //   the full parser...
  lemon_interface parse(lemon_interface::params().trace(p_.trace_parser()).context(NULL));
  flex_interface::init(flex_interface::params().stream(fh.get()).trace(p_.trace_lexer()));

  int tok;
  while ((tok = flex_interface::next_token())) {
    parse.token(tok);

    // TODO: check some state to work out if there was an error
  }

  parse.finish();

  // TODO:
  //   Return something sensible.
}

#endif
