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
    typedef ::stages::plugin_id_type plugin_id_type;

    // TODO:
    //   This class need to store paths and be able to deal with getting the
    //   loading done.  We need to know the complete category name and other
    //   such  stuff.

    stage_data() :
      plugin_id_(plug_id::unset)
    {}

    bool built_in() const {
      NERVE_ASSERT(plugin_id() != plug_id::unset, "plugin must be set before testing");
      return stages::is_built_in(this->plugin_id());
    }

    plugin_id_type plugin_id() const { return plugin_id_; }
    void plugin_id(plugin_id_type p) {
      NERVE_ASSERT(p != plug_id::unset, "can't set to the unset value");
      plugin_id_ = p;
    }

    private:
    plugin_id_type plugin_id_;
  };
}
#endif
