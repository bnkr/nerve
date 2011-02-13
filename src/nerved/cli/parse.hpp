#include "settings.hpp"

namespace cli {
  enum parse_status {
    parse_ok,
    parse_fail,
    parse_exit
  };

  //! \ingroup grp_cli
  //! Sets up the parser.
  parse_status parse(settings &s, int argc, char **argv);
}
