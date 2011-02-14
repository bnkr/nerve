// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef CONFIG_PARSE_HPP_4qe18k20
#define CONFIG_PARSE_HPP_4qe18k20
#include "pipeline_configs.hpp"

namespace cli { class settings; }

namespace config {
  enum parse_status {
    parse_fail,
    parse_ok
  };

  //! \ingroup grp_config
  //! Parses the configuration files supplied by the cli.  This is the entry
  //! point to the config module.
  parse_status parse(pipeline_config &, const cli::settings &);
}
#endif
