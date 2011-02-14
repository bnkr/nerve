// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef OUTPUT_CONFIGURE_HPP_tdxzgh0a
#define OUTPUT_CONFIGURE_HPP_tdxzgh0a
#include "logger.hpp"

namespace cli { class settings; }

namespace output {
  enum configure_status {
    configure_ok,
    configure_fail
  };

  //! \ingroup grp_output
  //! Called by main to configure the output settings.
  configure_status configure(logger &, const cli::settings &);
};
#endif
