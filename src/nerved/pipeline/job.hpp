// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_JOB_HPP_lih1nqby
#define PIPELINE_JOB_HPP_lih1nqby

#include "section.hpp"
#include "../util/pooled.hpp"
#include "../util/indirect.hpp"

namespace pipeline {
  struct section;
  struct pipe;

  /*!
   * \ingroup grp_pipeline
   *
   * Maps onto a thread, contains sections, and ensures we don't block too many
   * times in a thread (which causes deadlocks).
   */
  class job {
    public:
    typedef indirect_owned_monotype<section> sections_type;

    //! Returned pointer must remain valid.
    section *create_section(pipe *, pipe *);

    //! Called after all the "create" whatsits have been done.
    void finalise();

    //! Main method for a thread.
    void job_thread();

    private:
    sections_type &sections() { return sections_;  }

    sections_type sections_;
  };
}
#endif
