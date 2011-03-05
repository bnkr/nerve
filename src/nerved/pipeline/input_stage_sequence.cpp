// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "input_stage_sequence.hpp"

#include "../config/pipeline_configs.hpp"
#include "../stages/built_in_stages.hpp"

using namespace pipeline;

namespace {
  template<class T>
  T *do_alloc() { return (T*) pooled::tracked_byte_alloc(sizeof(T)); }
}


simple_stage *input_stage_sequence::create_stage(config::stage_config &cfg) {
  namespace plug_id = stages::plug_id;

  NERVE_ASSERT(is_ == NULL, "must not create an input stage twice");
  NERVE_NIMPL("input stage's stage addition");

  simple_stage *ret = NULL;

  switch (cfg.plugin_id()) {
  case plug_id::ffmpeg:
    ret = do_alloc<stages::ffmpeg>();
    break;
  case plug_id::plugin:
    NERVE_ABORT("don't know how to deal with a plugin");
    break;
  case plug_id::unset:
    NERVE_ABORT("unset stage id is impossible");
    break;
  case plug_id::sdl:
  case plug_id::volume:
    NERVE_ABORT("this type of stage cannot be handled by this sequence");
  }

  return NERVE_CHECK_PTR(ret);
}

stage_sequence::step_state input_stage_sequence::sequence_step() {
  packet *p = NERVE_CHECK_PTR(read_input());
  switch (p->event()) {
  case packet::event::load:
    load_event();
    break;
  case packet::event::skip:
    skip_event();
    break;
    // TODO : more events
  default:
    NERVE_ABORT("event should never happen here");
  }

  // change p.event to "abandon" unless we just read data
  write_output_wipe(p);

  return stage_sequence::state::complete;
}
// TODO:
//   Calls load on the input stage with details of what to load.  Not
//   entirely sure on that yet so let's leave it.
void input_stage_sequence::load_event() {
  NERVE_NIMPL("load file event");
}

void input_stage_sequence::skip_event() {
  NERVE_NIMPL("skip to timestamp event");
}
