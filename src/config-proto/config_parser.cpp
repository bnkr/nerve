// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "config_parser.hpp"
#include "lemon_interface.hpp"
#include "flex_interface.hpp"
#include "pipeline_configs.hpp"
#include "parse_context.hpp"
#include "semantic_checker.hpp"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>

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

  fh.reset(std::fopen(NERVE_CHECK_PTR(p_.file()), "r"));
  if (! fh.get() || std::ferror(fh.get())) {
    // TODO:
    //   exceptions?  I want to separate the output because often we'd want to
    //   write into a log file as well.  In theory the error_reporter class
    //   will use some kind of already declared strategy.  We can just use
    //   that here (or even it could be global if necessary).
    std::cerr << "could not open config file '" << p_.file() << "'" << std::endl;
    return false;
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

  semantic_pass(context);

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

// ///////////// //
// Semantic Pass //
// ///////////// //

void config_parser::semantic_pass(config::parse_context &ctx) {
  semantic_checker ch(ctx);
  ch.check();
}
