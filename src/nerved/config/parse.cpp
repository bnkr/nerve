#include "parse.hpp"

#include "../cli/settings.hpp"

config::parse_status config::parse(pipeline_config &pipes, const cli::settings &cli) {
  config::config_parser::params p;
  p.trace_parser(cli.trace_parser());
  p.trace_lexer(cli.trace_lexer());
  p.trace_general(p.trace_parser() || p.trace_lexer());

  config::config_parser parser(p);
  if (! parser.parse(pipes, cli.config_files())) {
    return parse_fail;
  }
  else {
    return parse_ok;
  }
}
