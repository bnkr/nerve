// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "config/parse.hpp"
#include "cli/parse.hpp"
#include "pipeline/configure.hpp"
#include "output/configure.hpp"
#include "player/run.hpp"

#include <cstdlib>

//! Links each bit of the daemon: cli, config, pipeline, and execution.
int main(int argc, char **argv) {
  cli::settings settings;
  switch (cli::parse(settings, argc, argv)) {
  case cli::parse_fail:
    return EXIT_FAILURE;
  case cli::parse_exit:
    return EXIT_SUCCESS;
  case cli::parse_ok:
    break;
  }

  output::logger log;
  switch (output::configure(log, settings)) {
  case output::configure_fail:
    return EXIT_FAILURE;
  case output::configure_ok:
    break;
  }

  // TODO:
  //   Should use a name which represents all configs (because there will
  //   undoubtedly be extra bits in the config file later)
  config::pipeline_config pipe_conf;
  switch(config::parse(pipe_conf, settings)) {
  case config::parse_fail:
    return EXIT_FAILURE;
  case config::parse_ok:
    break;
  }

  if (settings.dump_config()) {
    config::dump_config_yaml(pipe_conf);
    return EXIT_SUCCESS;
  }

  pipeline::pipeline_data pipe_data;
  switch (pipeline::configure(pipe_data, pipe_conf, settings)) {
  case pipeline::configure_fail:
    return EXIT_FAILURE;
  case pipeline::configure_ok:
    break;
  }

  switch (player::run(pipe_data, settings)) {
  case player::run_fail:
    return EXIT_FAILURE;
  case player::run_ok:
    break;
  }

  return EXIT_SUCCESS;
}
