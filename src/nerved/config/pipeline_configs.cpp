// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_configs.hpp"

#include <cstring>

using namespace config;

/****************
 * Stage config *
 ****************/

// TODO:
//   This should be handled by the stages module.

// This is not quite the same as converting the enumeration to a string (or at
// least it won't be when there's a loaded plugin) so it's defined here instead
// of where the stage ids are defined.  All that stuff is subject to change when
// we have the proper work done for loading internally built or shared object
// loaded plugins.
const char *stage_config::get_stage_name(const stage_config &s) {
  if (s.stage_data().built_in()) {
    return ::stages::get_plugin_enum_name(s.plugin_id());
  }
  else {
    // TODO:
    //   This should be handled by stage_data somehow.
    return "(unknown plugin)";
  }
}

// TODO:
//   This shouldbe handled bythe stage data somehow.
stage_config::category_type stage_config::category() const {
  NERVE_ASSERT(this->plugin_id() != plug_id::unset, "don't call this until the path/id is done");

  if (this->stage_data().built_in()) {
    return stages::get_built_in_plugin_category(this->plugin_id());
  }
  else {
    NERVE_ABORT("not implemented: finding the category from a plugin path");
  }
}

/********************
 * Key-Value Blocks *
 ********************/

template<class Pair>
struct destroy_snd {
  void operator()(Pair &p) const {
    return configure_block::destroy(p.second);
  }
};

void configure_block_container::destroy_blocks() {
  std::for_each(
    blocks().begin(), blocks().end(),
    destroy_snd<blocks_type::value_type>()
  );
}

/*******************
 * Pipeline Config *
 *******************/

void pipeline_config::madagascar() {
  jobs().clear();
  configure_blocks().clear();
}
