// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "section.hpp"

using namespace pipeline;

void section::finalise() {
  NERVE_ASSERT(! sequences().empty(), "there must be some sequences");
  std::for_each(sequences().begin(), sequences().end(), boost::bind(&stage_sequence::finalise, _1));
  reset_start();
}

stage_sequence *section::create_sequence(section::category_type c) {
  std::cerr << __PRETTY_FUNCTION__ << ": returning null" << std::endl;
  return NULL;
}

void section::section_step() {
  bool do_reset = true;
  typedef sequence_type::state state;

  for (iterator_type s = start(); s != this->sequences().end(); ++s) {
    // See class docs for why this is the least bad solution.
    sequence_type::step_state ret = NERVE_CHECK_PTR(*s)->sequence_step();

    if (ret == state::buffering) {
      // TODO:
      //   If there are multiple sequences doing buffering, then we ignore one
      //   of them here.  There needs to be a stack of iterators.
      start(s);
      do_reset = false;
    }
    else {
      NERVE_ASSERT(ret == state::complete, "there are no other possibilities");
    }
  }

  if (do_reset) {
    reset_start();
  }
}

