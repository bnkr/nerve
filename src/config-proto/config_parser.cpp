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
#include <boost/bind.hpp>

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
}

bool config_parser::parse(config::pipeline_config &output, const config_parser::files_type &fs) {
  parse_context context(output);
  std::for_each(fs.begin(), fs.end(), boost::bind(&config_parser::parse_file, this, boost::ref(context), _1));
  return ! context.reporter().error();
}

bool config_parser::parse_file(config::parse_context &context, const char *file) {
  stdio_ptr fh;

  context.reporter().location().new_file(file);

  fh.reset(std::fopen(NERVE_CHECK_PTR(file), "r"));
  if (! fh.get() || std::ferror(fh.get())) {
    context.reporter().report("file not readable");
    return false;
  }

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

  if (! context.reporter().error()) {
    semantic_pass(context);
  }

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
  ch.register_names();
  ch.link_pipeline_order();
  ch.link_jobs_and_check_stage_order();
}
