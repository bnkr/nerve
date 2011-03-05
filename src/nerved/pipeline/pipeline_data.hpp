// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PIPELINE_DATA_HPP_04jolpd6
#define PIPELINE_PIPELINE_DATA_HPP_04jolpd6

#include <vector>
#include <boost/utility.hpp>

namespace pipeline {
  struct job;
  struct connector;

  //! \ingroup grp_pipeline
  //! Container for the initialised pipeline.
  class pipeline_data : boost::noncopyable {
    public:

    ~pipeline_data();

    job *create_job();

    //! The start and end pipes for the entire pipeline.
    connector *start_terminator();
    connector *end_terminator();

    //! Gets a new pipe to connect multi-threaded sections.
    connector *create_pipe();

    //! Check and finish the pipeline objects after configuration.
    void finalise();

    private:
    std::vector<job*> jobs_;
  };
}

#endif
