// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"
#include "../cli/settings.hpp"
#include "../config/pipeline_configs.hpp"

#include <iostream>

using namespace pipeline;

configure_status pipeline::configure(pipeline_data &, const config::pipeline_config &, const cli::settings &) {
  std::cerr << "not implemented" << std::endl;
  return configure_fail;
}
