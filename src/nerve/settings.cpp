#include "settings.hpp"

#include <nerve_config.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <cstdlib>

void settings::print_version() const {
  std::cout <<
    "nerve version " NERVE_VERSION ", compiled on " __DATE__ " for " NERVE_TARGET_SYSTEM ".\n"
    "Copyright (C) James Webber 2009.  Distributed under a 3-clause BSD licence.\n"
    << std::flush;
}

void settings::print_help(boost::program_options::options_description &opts) const {
  std::cout <<
    "Usage: nerve command [option]...\n"
    "Controls the nerve daemon's playlist and running state.\n";

  std::cout << opts << std::endl;
  print_version();
}

void settings::load_options(int argc, char **argv) {
  namespace po = boost::program_options;

  po::options_description general_opts("General options");
  general_opts.add_options()
    ("help", "Display options description and exit.")
    ("version", "Display version and copyright and exit.")
    ;

  po::options_description all_opts;
  all_opts
    .add(general_opts);

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc, argv).options(all_opts).run();
  po::store(parsed, vm);

  if (vm.count("help")) {
    exit_status_ = EXIT_SUCCESS;
    print_help(all_opts);
    return;
  }
  else if (vm.count("version")) {
    exit_status_ = EXIT_SUCCESS;
    print_version();
    return;
  }
}

void settings::set_defaults() {
  exit_status_ = -1;
}
