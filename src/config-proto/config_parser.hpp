// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * The config file parser.  This combines the flex and lemon bits into stuff we
 * actually want to use.
 */

#ifndef CONFIG_CONFIG_PARSER_HPP_999mx4dk
#define CONFIG_CONFIG_PARSER_HPP_999mx4dk

#include "../plugin-proto/asserts.hpp"

namespace config {
  class pipeline_config;

  //! Main interface to configuration parsing.
  class config_parser {
    public:

    struct params {
      params()
        : file_("-"),
          trace_general_(false),
          trace_parser_(false),
          trace_lexer_(false),
          lexer_only_(false)
      {}

      const char *file() const { return file_; }
      bool trace_general() const { return trace_general_; }
      bool trace_parser() const { return trace_parser_; }
      bool trace_lexer() const { return trace_lexer_; }
      bool lexer_only() const { return lexer_only_; }

      params &file(const char *v) { NERVE_ASSERT_PTR(v); file_ = v; return *this; }
      params &trace_general(bool v) { trace_general_ = v; return *this; }
      params &trace_parser(bool v) { trace_parser_ = v; return *this; }
      params &trace_lexer(bool v) { trace_lexer_ = v; return *this; }
      params &lexer_only(bool v) { lexer_only_ = v; return *this; }

      private:
      const char *file_;
      // TODO: use bitset and enum
      bool trace_general_;
      bool trace_parser_;
      bool trace_lexer_;
      bool lexer_only_;
    };

    config_parser(const params &p);
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

using config::config_parser;

config_parser::config_parser(const config_parser::params &p) : p_(p) {
  NERVE_ASSERT(p_.file() != NULL, "file must never be null");
}

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

void config_parser::parse(config::pipeline_config &output) {
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
