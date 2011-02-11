// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "config_parser.hpp"
#include "pipeline_configs.hpp"

#include <iostream>
#include <vector>
#include <cstdio>

using config::config_parser;

int parse_args(config_parser::params &p, config_parser::files_type &files, int argc, char **argv) {
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
    else {
      files.push_back(argv[i]);
    }
  }

  if (files.empty()) {
    std::cout << argv[0] << ": no file given" << std::endl;
    return false;
  }

  return true;
}

int wrapped_main(int argc, char **argv) {
  config_parser::files_type files;
  files.reserve(16);
  config::config_parser::params p;
  if (! parse_args(p, files, argc, argv)) {
    return EXIT_FAILURE;
  }

  config::config_parser parser(p);
  config::pipeline_config pipes;

  if (! parser.parse(pipes, files)) {
    return EXIT_FAILURE;
  }

  config::dump_config_yaml(pipes);

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  // So we don't have any accessable pointers.
  return wrapped_main(argc, argv);
}
