// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PIPELINE_DATA_HPP_04jolpd6
#define PIPELINE_PIPELINE_DATA_HPP_04jolpd6

// Necessary because it's destroyed in the pooled_destructor so the size is
// needed.
#include "job.hpp"
#include "terminators.hpp"

#include <boost/utility.hpp>
#include "../util/pooled.hpp"
#include "../util/indirect.hpp"

namespace pipeline {
  struct job;
  struct pipe;

  //! \ingroup grp_pipeline
  //! Container for the initialised pipeline.
  class pipeline_data : boost::noncopyable {
    public:
    typedef indirect_owned_monotype<job> jobs_type;

    //! A new job container owned by this object.  Pointer is valid
    //! indefinitely.
    job *create_job();

    //! The start and end pipes for the entire pipeline.
    pipeline::start_terminator *start_terminator() { return &start_terminator_; }
    pipeline::end_terminator *end_terminator() { return &end_terminator_; }

    //! Check and finish the pipeline objects after configuration.
    void finalise();

    //! Remove all memory etc.
    void clear();

    const jobs_type &jobs() const { return jobs_; }
    jobs_type &jobs() { return jobs_; }

    private:
    jobs_type jobs_;
    pipeline::start_terminator start_terminator_;
    pipeline::end_terminator end_terminator_;
  };
}

#endif
