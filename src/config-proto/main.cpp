// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "config_parser.hpp"
#include "pipeline_configs.hpp"
#include "make_pipeline.hpp"

#include <iostream>
#include <cstdio>

int parse_args(config::config_parser::params &p, int argc, char **argv) {
  bool file_given = false;
  for (int i = 1; i != argc; ++i) {
    if (std::strcmp(argv[i], "-p") == 0) {
      p.trace_parser(true);
    }
    else if (std::strcmp(argv[i], "-l") == 0) {
      p.trace_lexer(true);
    }
    else if (argv[i][0] == '-') {
      std::cout << argv[0] << ": unrecognised argument: " << argv[i] << std::endl;
      return false;
    }
    else if (file_given) {
      std::cout << argv[0] << ": " << argv[i] << ": file already given" << std::endl;
      return false;
    }
    else {
      file_given = true;
      p.file(argv[i]);
    }
  }

  if (! p.file()) {
    std::cout << argv[0] << ": no file given" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char **argv) {
  config::config_parser::params p;
  if (! parse_args(p, argc, argv)) {
    return EXIT_FAILURE;
  }

  config::config_parser parser(p);
  config::pipeline_config pipes;

  if (! parser.parse(pipes)) {
    return EXIT_FAILURE;
  }

  make_pipeline(pipes);

  return EXIT_SUCCESS;
}
