// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "create.hpp"

#include "built_in_stages.hpp"
#include "stage_data.hpp"
#include "../pipeline/simple_stages.hpp"

#include "../util/asserts.hpp"

using namespace stages;

using pipeline::input_stage;

namespace {
  template<class T, class Function>
  T *allocate(Function f) {
    void *const p = f(sizeof(T));
    T *const pt = (T *) p;
    new (pt) T();
    return pt;
  }

  pipeline::simple_stage *create_stage(stage_data &sd, alloc_func alloc) {
    if (sd.built_in()) {
      switch (sd.plugin_id()) {
      case plug_id::ffmpeg:
        return allocate<stages::ffmpeg>(alloc);
      case plug_id::sdl:
        return allocate<stages::sdl>(alloc);
      case plug_id::volume:
        NERVE_ABORT("volume builtin not handled yet");
      case plug_id::plugin:
      case plug_id::unset:
        NERVE_ABORT("impossible value for built-in plugin");
      }

      NERVE_ABORT("general impossibility");
    }
    else {
      NERVE_ABORT("don't know how to deal with a plugin yet");
      return NULL;
    }
  }
}

// All we want is some type safety.
pipeline::input_stage *stages::create_input_stage(stage_data &sd, alloc_func alloc) {
  // TODO:
  //   Assert that the plugin really is an input.
  pipeline::simple_stage *const ret = NERVE_CHECK_PTR(create_stage(sd, alloc));
  return static_cast<pipeline::input_stage*>(ret);
}

pipeline::observer_stage *stages::create_observer_stage(stage_data &sd, alloc_func alloc) {
  // TODO:
  //   Assert that the plugin really is an observer.
  pipeline::simple_stage *const ret = NERVE_CHECK_PTR(create_stage(sd, alloc));
  return static_cast<pipeline::observer_stage*>(ret);
}
