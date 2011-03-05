// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PIPELINE_DATA_HPP_04jolpd6
#define PIPELINE_PIPELINE_DATA_HPP_04jolpd6

// necessary because it's destroyed in the pooled_destructor so the size is
// needed.
//
// TODO:
//   Could be a workaround for this?
#include "job.hpp"

#include <boost/utility.hpp>
#include "../util/pooled.hpp"
#include "../util/indirect.hpp"

namespace pipeline {
  struct job;
  struct connector;

  //! \ingroup grp_pipeline
  //! Container for the initialised pipeline.
  class pipeline_data : boost::noncopyable {
    public:

    //! A new job container owned by this object.  Pointer is valid
    //! indefinitely.
    job *create_job();

    //! The start and end pipes for the entire pipeline.
    connector *start_terminator();
    connector *end_terminator();

    //! Gets a new pipe to connect multi-threaded sections.
    connector *create_pipe();

    //! Check and finish the pipeline objects after configuration.
    void finalise();

    private:
    typedef pooled::container<job*>::vector vector_type;
    typedef indirect_owned<vector_type, pooled_destructor> jobs_type;

    jobs_type jobs_;
  };
}

#endif
