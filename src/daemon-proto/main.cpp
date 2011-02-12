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
  switch (output::configure(log, cli)) {
  case output::configure_fail:
    return EXIT_FAILURE;
  case output::configure_ok:
    break;
  }

  config::pipeline_config pipe_conf;
  switch(config::parse(pipe_conf, cli)) {
  case config::parse_fail:
    return EXIT_FAILURE;
  case config::parse_exit:
    return EXIT_SUCCESS;
  case config::parse_ok:
    break;
  }

  pipeline::pipeline pipe;
  switch (pipeline::configure(pipe, pipe_conf)) {
  case pipeline::configure_fail:
    return EXIT_FAILURE;
  case pipeline::configure_ok:
    break;
  }

  switch (player::run(pipe)) {
  case player::run_fail:
    return EXIT_FAILURE;
  case player::run_ok:
    break;
  }

  return EXIT_SUCCESS;
}
