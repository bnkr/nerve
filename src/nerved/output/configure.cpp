// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"

#include "logging.hpp"
#include "../cli/settings.hpp"

#include <iostream>

output::configure_status output::configure(const cli::settings &s) {
  // TODO:
  //   Very incomplete, obviously.
  detail::log_data &ld = detail::get_data();
  ld.console(stderr);
  return configure_ok;
}
