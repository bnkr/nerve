// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "process_stage_sequence.hpp"

#include "../stages/stage_data.hpp"

using namespace pipeline;

simple_stage *process_stage_sequence::create_stage(stages::stage_data &cfg) {
  NERVE_NIMPL("creating a process stage sequence member");
  return NULL;
}

void process_stage_sequence::finalise() {
  data_loop_.finalise();
}

stage_sequence::step_state process_stage_sequence::sequence_step() {
  typedef stage_sequence::state state;

  if (data_loop_.buffering()) {
    // We must check for wipe events first to reduce latency.  Otherwise a
    // buffering stage will continue to do work while there is an abandon event
    // on the queue.
    packet *p = connection().read_input_wipe();
    if (p) {
      NERVE_ASSERT(p->event() == packet::event::abandon, "only 'abandon' packets cause wipes");
      data_loop_.abandon_reset();
      this->non_data_step(stages(), p);
      return state::complete;
    }
    else {
      data_loop_.step();
      return data_loop_.buffering() ? state::buffering : state::complete;
    }
  }
  else {
    data_loop_.step();
    return data_loop_.buffering() ? state::buffering : state::complete;
  }
}


