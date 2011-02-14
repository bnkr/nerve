// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"

#include "../cli/settings.hpp"

#include <iostream>

output::configure_status output::configure(output::logger &lg, const cli::settings &s) {
  std::cout << "output::configure not implemented" << std::endl;
  return configure_fail;
}
