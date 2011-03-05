// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "process_stage_sequence.hpp"

using namespace pipeline;

simple_stage *process_stage_sequence::create_stage(config::stage_config &cfg) {
  NERVE_NIMPL("creating a process stage sequence member");
  return NULL;
}

void process_stage_sequence::finalise() {
  data_loop_.finalise();
}

stage_sequence::step_state process_stage_sequence::sequence_step() {
  typedef stage_sequence::state state;

  // TODO:
  //   We must check for non-data events first to reduce latency.  Otherwise a
  //   buffering stage will continue to do work while there is an abandon event
  //   on the queue.  That said, it's tricky to do because some events (such as
  //   a flush) needs to stay in order.
  packet *p = NERVE_CHECK_PTR(read_input());
  switch (p->event()) {
  case packet::event::data:
    data_loop_.step();
    return data_loop_.buffering() ? state::buffering : state::complete;
  case packet::event::abandon:
    data_loop_.abandon_reset();
    // fallthrough
  default:
    this->non_data_step(stages(), p);
    return state::complete;
  }
}


