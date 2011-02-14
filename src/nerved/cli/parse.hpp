// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef CLI_PARSE_HPP_utyn8koh
#define CLI_PARSE_HPP_utyn8koh
#include "settings.hpp"

namespace cli {
  enum parse_status {
    parse_ok,
    parse_fail,
    parse_exit
  };

  //! \ingroup grp_cli
  //! Sets up the parser and stores settings data.
  parse_status parse(settings &s, int argc, char **argv);
}
#endif
