// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_configs.hpp"

#include <cstring>

using namespace config;

/****************
 * Stage config *
 ****************/

// TODO:
//
//   Should probably be dealt with in pipeline somewhere because that's where
//   the enumeration is defined.  It depends how plugins are eventually loaded.
//   Prolly it'll all move around a bit.

stage_config::plugin_id_type stage_config::get_plugin_id(const char *name) {
  NERVE_ASSERT_PTR(name);

  if (std::strcmp(name, "sdl") == 0) {
    return plug_id::sdl;
  }
  else if (std::strcmp(name, "ffmpeg") == 0) {
    return plug_id::ffmpeg;
  }
  else if (std::strcmp(name, "volume") == 0) {
    return plug_id::volume;
  }
  else {
    return plug_id::unset;
  }
}

// This is not quite the same as converting the enumeration to a string (or at
// least it won't be when there's a loaded plugin) so it's defined here instead
// of where the stage ids are defined.  All that stuff is subject to change when
// we have the proper work done for loading internally built or shared object
// loaded plugins.
const char *stage_config::get_stage_name(const stage_config &s) {
  switch (s.plugin_id()) {
  case plug_id::unset:
    // TODO:
    //   This means the object is not being constructed by its constructor.
    //   Instead we should have stage_config and stage_data wherein the second
    //   is only declared when everything else is done.
    NERVE_ABORT("don't call this until the path/id is done");
  case plug_id::plugin:
    NERVE_ABORT("not implemented: finding the stage name from a plugin path");
  case plug_id::ffmpeg:
    return "ffmpeg";
  case plug_id::sdl:
    return "sdl";
  case plug_id::volume:
    return "volume";
  }

  NERVE_ABORT("impossible!");
}

// TODO:
//   Bit of a placeholder.
stage_config::category_type stage_config::category() const {
  switch (this->plugin_id()) {
  case plug_id::unset:
    NERVE_ABORT("don't call this until the path/id is done");
  case plug_id::plugin:
    NERVE_ABORT("not implemented: finding the category from a plugin path");
  case plug_id::ffmpeg:
    return stage_cat::input;
  case plug_id::sdl:
    return stage_cat::output;
  case plug_id::volume:
    return stage_cat::process;
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
