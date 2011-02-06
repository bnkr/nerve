// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "config_parser.hpp"
#include "pipeline_configs.hpp"

#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << argv[0] << ": not enough args" << std::endl;
    return 1;
  }

  config::config_parser::params p;
  p.trace_lexer(true);
  config::config_parser parser(p.file(argv[1]));

  config::pipeline_config pipes;
  parser.parse(pipes);
}
