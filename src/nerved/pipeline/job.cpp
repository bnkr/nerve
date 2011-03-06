// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "job.hpp"
#include "section.hpp"

#include "../util/asserts.hpp"

using namespace pipeline;

void job::finalise() {
  std::for_each(sections().begin(), sections().end(), boost::bind(&section::finalise, _1));
}

section *job::create_section(pipe *in, pipe *out) {
  section *const s = sections_.alloc_back();
  s->connection().in(in);
  s->connection().out(out);
  return s;
}

void pipeline::job::job_thread() {
forever:
  typedef sections_type::iterator iter_type;
  bool blocked_already = false;
  for (iter_type s = sections().begin(); s != sections().end(); ++s) {
    // Fullfills the "jobs mustn't block twice" requirement.
    //
    // TODO:
    //   The mechanism and purpose is not well defined.  Especially we might
    //   care if blocking on input or output.  It might be that we do it
    //   based on the return from section_step and let the messy details of
    //   that be totally transparent.
    //
    if (s->would_block()) {
      if (blocked_already) {
        // Continue instead of break to avoid a bias towards sections at the
        // start of the job.
        continue;
      }
      else {
        blocked_already = true;
      }
    }

    s->section_step();
  }

  goto forever;
}

