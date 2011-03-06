// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "section.hpp"

#include "observer_stage_sequence.hpp"
#include "process_stage_sequence.hpp"
#include "input_stage_sequence.hpp"

using namespace pipeline;

void section::finalise() {
  NERVE_ASSERT(! sequences().empty(), "there must be some sequences");
  std::for_each(sequences().begin(), sequences().end(), boost::bind(&stage_sequence::finalise, _1));
  reset_start();
}

namespace {
  stage_sequence *alloc_and_construct(section::sequences_type &seq, stages::category_type c) {
    namespace stage_cat = ::stages::stage_cat;

    switch (c) {
    case stage_cat::observe:
      return seq.alloc_back<observer_stage_sequence>();
    case stage_cat::process:
      return seq.alloc_back<process_stage_sequence>();
    case stage_cat::input:
      return seq.alloc_back<input_stage_sequence>();
    case stage_cat::output:
      // TODO:
      //   Output and observes are identical, so this should *not* create a new
      //   sequence -- it should return the previous one.
      return seq.alloc_back<output_stage_sequence>();
    case stage_cat::unset:
      NERVE_ABORT("unset should be impossible");
      break;
    }

    NERVE_ABORT("should be impossible to get here");
  }
}

namespace {
  // disambiguate
  typedef pipeline::pipe pipe_type;
}

stage_sequence *section::create_sequence(stages::category_type c, pipe_type *in, pipe_type *out) {
  stage_sequence *const s = NERVE_CHECK_PTR(alloc_and_construct(this->sequences(), c));
  s->connection().in(in);
  s->connection().out(out);
  return s;
}

void section::section_step() {
  bool do_reset = true;
  typedef sequence_type::state state;

  for (iterator_type s = start(); s != this->sequences().end(); ++s) {
    // See class docs for why this is the least bad solution.
    sequence_type::step_state ret = s->sequence_step();

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

