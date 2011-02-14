// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \ingroup grp_pipeline
 * Entry point to the pipeline module.
 */

#ifndef PIPELINE_CONFIGURE_HPP_p4yej2k6
#define PIPELINE_CONFIGURE_HPP_p4yej2k6
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
  configure_status configure(pipeline_data &, config::pipeline_config &, const cli::settings &);
}
#endif
