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

#include <boost/utility.hpp>

struct syntactic_context;

namespace config {
  class pipeline_config;
  class parse_context;

  //! Main interface to configuration parsing.
  class config_parser : boost::noncopyable {
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
    bool parse(pipeline_config &output);

    private:

    void syntactic_pass(struct syntactic_context &);
    void semantic_pass(pipeline_config &);

    params p_;
  };
}

#include "lemon_interface.hpp"
#include "flex_interface.hpp"
#include "pipeline_configs.hpp"
#include "parse_context.hpp"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>

using config::config_parser;

// //////////// //
// Utility Bits //
// //////////// //

//! Stdio with RAII.
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

//! Simple aggregator.
struct syntactic_context {
  syntactic_context(config::parse_context &pc, config::lemon_interface &li)
  : parse_context(pc), parser(li)  {
  }

  config::parse_context   &parse_context;
  config::lemon_interface &parser;
};

// ////////////// //
// Initialisation //
// ////////////// //

config_parser::config_parser(const config_parser::params &p) : p_(p) {
  NERVE_ASSERT(p_.file() != NULL, "file must never be null");
}

bool config_parser::parse(config::pipeline_config &output) {
  stdio_ptr fh;

  if (std::strcmp(p_.file(), "-") != 0) {
    fh.reset(std::fopen(p_.file(), "r"));
    if (! fh.get() || std::ferror(fh.get())) {
      // TODO:
      //   exceptions?  I want to separate the output because often we'd want to
      //   write into a log file as well.  In theory the error_reporter class
      //   will use some kind of already declared strategy.  We can just use
      //   that here (or even it could be global if necessary).
      std::cerr << "xxsc-compile: could not open file '" << p_.file() << "'" << std::endl;
      return false;
    }
  }

  parse_context context(output);

  context.reporter().location().new_file(p_.file());

  {
    lemon_interface parse = lemon_interface::params()
      .trace(p_.trace_parser())
      .context(&context);

    flex_interface::params fp = flex_interface::params()
      .trace(p_.trace_lexer())
      .stream(fh.get())
      .context(&context);
    flex_interface::init(fp);

    syntactic_context syn(context, parse);
    syntactic_pass(syn);
  }

  if (context.reporter().error()) {
    return false;
  }

  semantic_pass(output);

  return ! context.reporter().error();
}

// ////////////////// //
// Parsing Operations //
// ////////////////// //


void config_parser::syntactic_pass(syntactic_context &syn) {
  int tok;
  while ((tok = flex_interface::next_token())) {
    // The lexer doesn't return if it can't deal with the character so it's ok
    // to carry on.
    syn.parser.token(tok);
    if (syn.parse_context.reporter().fatal_error()) {
      return;
    }
  }

  syn.parser.finish();
}

void config_parser::semantic_pass(config::pipeline_config &confs) {
  std::map<std::string, section_config*> names;

  typedef pipeline_config::job_iterator_type   job_iter_t;
  typedef job_config::section_iterator_type    section_iter_t;

  for (job_iter_t job = confs.begin(); job != confs.end(); ++job) {
    for (section_iter_t sec = job->begin(); sec != job->end(); ++sec) {
      names[std::string(sec->name())] = &(*sec);

      if (names.count(sec->after_name())) {
        sec->after_section(names[std::string(sec->after_name())]);
      }
    }
  }


}

#endif
