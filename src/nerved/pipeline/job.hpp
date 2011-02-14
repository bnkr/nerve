// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_JOB_HPP_lih1nqby
#define PIPELINE_JOB_HPP_lih1nqby

#include "section.hpp"

#include <vector>

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   *  Maps onto a thread, contains sections, and ensures we don't block too many
   * times in a thread (which causes deadlocks).
   */
  class job {
    public:

    typedef std::vector<section> sections_type;

    void job_thread() {
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

    private:
    sections_type &sections() { return sections_;  }

    sections_type sections_;
  };
}
#endif
