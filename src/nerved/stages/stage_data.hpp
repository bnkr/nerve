// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_STAGE_DATA_HPP_smp9ri8t
#define STAGES_STAGE_DATA_HPP_smp9ri8t

#include "information.hpp"

#include "../util/asserts.hpp"

namespace stages {
  /*!
   * \ingroup grp_stages
   * The data necessary to configure and load a built-in stage or plugin.  This
   * is used by the pipeline configuration procedure (the create_stage methods)
   * and by the config objects.
   */
  class stage_data {
    public:
    stage_data() :
      plugin_id_(plug_id::unset)
    {}

    bool built_in() const {
      NERVE_ASSERT(plugin_id() != plug_id::unset, "plugin must be set before testing");
      return stages::built_in_plugin(this->plugin_id());
    }

    plugin_id_type plugin_id() const { return plugin_id_; }

    private:
    plugin_id_type plugin_id_;
  };
}
#endif
