/*!
 * \ingroup grp_pipeline
 * Entry point to the pipeline module.
 */

#include "pipeline_data.hpp"

namespace config { class pipeline_config; }
namespace cli { class settings; }

namespace pipeline {
  enum configure_status {
    configure_ok,
    configure_fail
  };

  //! \ingroup grp_pipeline
  //! Create the pipeline based on the config.
  configure_status configure(pipeline_data &, const config::pipeline_config &, const cli::settings &);
}
