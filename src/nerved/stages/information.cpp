// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "information.hpp"

#include "../util/asserts.hpp"

#include <cstring>

using namespace stages;

const char *stages::get_category_name(stages::category_type c) {
  switch (c) {
  case stage_cat::input:
    return "input";
  case stage_cat::output:
    return "output";
  case stage_cat::process:
    return "process";
  case stage_cat::observe:
    return "observe";
  case stage_cat::unset:
    return "(undefined)";
  }

  NERVE_ABORT("impossible value for category_type");
}

plugin_id_type stages::get_built_in_plugin_id(const char *name) {
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

const char *stages::get_plugin_enum_name(stages::plugin_id_type p) {
  switch (p) {
  case plug_id::sdl:
    return "sdl";
  case plug_id::ffmpeg:
    return "ffmpeg";
  case plug_id::volume:
    return "volume";
  case plug_id::unset:
    return "(unset)";
  case plug_id::plugin:
    return "(plugin)";
  }

  NERVE_ABORT("impossible value for plugin_id_type");
}

category_type stages::get_built_in_plugin_category(plugin_id_type p) {
  switch (p) {
  case plug_id::unset:
  case plug_id::plugin:
    return stage_cat::unset;
  case plug_id::sdl:
    return stage_cat::output;
  case plug_id::ffmpeg:
    return stage_cat::input;
  case plug_id::volume:
    return stage_cat::process;
  }

  NERVE_ABORT("impossible value of plugin_id_type");
}
