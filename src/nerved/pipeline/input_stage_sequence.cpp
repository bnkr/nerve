// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "input_stage_sequence.hpp"

#include "../stages/create.hpp"
#include "../stages/stage_data.hpp"

using namespace pipeline;

simple_stage *input_stage_sequence::create_stage(stages::stage_data &cfg) {
  NERVE_ASSERT(is_ == NULL, "must not create an input stage twice");
  return is_ = NERVE_CHECK_PTR(::stages::create_input_stage(cfg));
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
  case packet::event::data:
    p = NERVE_CHECK_PTR(is_->read());
    break;
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
