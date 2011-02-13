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
