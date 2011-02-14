// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_configs.hpp"

using namespace config;

/****************
 * Stage config *
 ****************/

const char *stage_config::get_category_name(stage_config::category_type c) {
  switch (c) {
  case cat_input:
    return "input";
  case cat_output:
    return "output";
  case cat_process:
    return "process";
  case cat_observe:
    return "observe";
  case cat_unset:
    return "(undefined)";
  }

  NERVE_ABORT("what are you doing here?");
}

stage_config::plugin_id_type stage_config::get_plugin_id(const char *name) {
  NERVE_ASSERT_PTR(name);

  if (std::strcmp(name, "sdl") == 0) {
    return stage_config::id_sdl;
  }
  else if (std::strcmp(name, "ffmpeg") == 0) {
    return stage_config::id_ffmpeg;
  }
  else if (std::strcmp(name, "volume") == 0) {
    return stage_config::id_volume;
  }
  else {
    return stage_config::id_unset;
  }
}

const char *stage_config::get_stage_name(const stage_config &s) {
  switch (s.plugin_id()) {
  case id_unset:
    // TODO:
    //   This means the object is not being constructed by its constructor.
    //   Instead we should have stage_config and stage_data wherein the second
    //   is only declared when everything else is done.
    NERVE_ABORT("don't call this until the path/id is done");
  case id_plugin:
    NERVE_ABORT("not implemented: finding the stage name from a plugin path");
  case id_ffmpeg:
    return "ffmpeg";
  case id_sdl:
    return "sdl";
  case id_volume:
    return "volume";
  }

  NERVE_ABORT("impossible!");
}

stage_config::category_type stage_config::category() const {
  switch (this->plugin_id()) {
  case id_unset:
    NERVE_ABORT("don't call this until the path/id is done");
  case id_plugin:
    NERVE_ABORT("not implemented: finding the category from a plugin path");
  case id_ffmpeg:
    return cat_input;
  case id_sdl:
    return cat_output;
  case id_volume:
    return cat_process;
  }

  NERVE_ABORT("what are you doing here?");
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
