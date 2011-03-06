// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "run.hpp"

#include "../output/logging.hpp"

player::run_status player::run(pipeline::pipeline_data &, const cli::settings &) {
  output::logger log(output::source::player);
  log.trace("player starting\n");

  log.fatal("player not implemented\n");
  return run_fail;
}
