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

  /*!
   * \ingroup grp_pipeline
   *
   * Create the pipeline based on the config.
   *
   * The convention here is to call x = pipe_obj.create_x (with some parameters
   * related to a config object), modify x by reference and then later call some
   * validation (or do it in the modification functions).  This means that the
   * objects have the potential to be invalid at certain times, but also means
   * we can abstract all the memory managment and ownership to the objects which
   * are doing the containing.  Since there are relatively few code paths
   * through this configuration, I rate this as a nice trade off, especially
   * since it makes the code quite a bit more simple.
   */
  configure_status configure(pipeline_data &, config::pipeline_config &, const cli::settings &);
}
#endif
