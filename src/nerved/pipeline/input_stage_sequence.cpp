// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "input_stage_sequence.hpp"

#include "../config/pipeline_configs.hpp"
#include "../stages/create.hpp"
#include "../stages/stage_data.hpp"

using namespace pipeline;

simple_stage *input_stage_sequence::create_stage(config::stage_config &cfg) {
  NERVE_ASSERT(is_ == NULL, "must not create an input stage twice");
  // TODO:
  //   This object should be in the stage_config object.  Loading plugins is
  //   still not 100% clear, but it's going to be done mostly by stage_data.
  stages::stage_data conf;
  return is_ = NERVE_CHECK_PTR(stages::create_input_stage(conf));
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
