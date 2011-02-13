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
