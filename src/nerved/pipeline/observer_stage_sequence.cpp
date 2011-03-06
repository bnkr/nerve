// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "observer_stage_sequence.hpp"

#include "packet.hpp"
#include "simple_stages.hpp"

#include <algorithm>
#include <boost/bind.hpp>

using namespace pipeline;

stage_sequence::step_state observer_stage_sequence::sequence_step() {
  packet *p = read_input();

  switch(NERVE_CHECK_PTR(p)->event()) {
  case packet::event::data:
    std::for_each(
      stages().begin(), stages().end(),
      boost::bind(&observer_stage::observe, _1, p)
    );
    write_output(p);
    break;
  default:
    this->non_data_step(stages(), p);
    break;
  }

  return stage_sequence::state::complete;
}

simple_stage *observer_stage_sequence::create_stage(stages::stage_data &cfg) {
  NERVE_NIMPL("creating a stage in an observer sequence");
  return NULL;
}
