#include "parse.hpp"

config::parse_status config::parse() {
  config_parser::files_type files;
  config::config_parser::params p;
  p.trace_parser(cli.trace_parser());
  p.trace_lexer(cli.trace_lexer());
  p.trace_general(p.trace_parser() || p.trace_lexer());

  config::config_parser parser(p);
  config::pipeline_config pipes;

  if (! parser.parse(pipes, files)) {
    return parse_fail;
  }
  else {
    return parse_ok;
  }
}
