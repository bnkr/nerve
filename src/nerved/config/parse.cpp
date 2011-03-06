// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "parse.hpp"

#include "../cli/settings.hpp"
#include "../output/logging.hpp"

config::parse_status config::parse(pipeline_config &pipes, const cli::settings &cli) {
  output::logger lg(output::source::config);
  lg.trace("parsing configuration files\n");

  config::config_parser::params p;
  p.trace_parser(cli.trace_parser());
  p.trace_lexer(cli.trace_lexer());
  p.trace_general(p.trace_parser() || p.trace_lexer());

  config::config_parser parser(p);
  if (! parser.parse(pipes, cli.config_files())) {
    lg.fatal("errors in configuration\n");
    return parse_fail;
  }
  else {
    return parse_ok;
  }
}
