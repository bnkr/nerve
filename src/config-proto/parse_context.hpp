// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "error_reporter.hpp"
#include "pipeline_configs.hpp"
#include "../plugin-proto/asserts.hpp"

#include <boost/utility.hpp>

namespace config {
  class pipeline_config;

  //! \ingroup grp_config
  //! Parsing context used by the lexer and parser.
  class parse_context : boost::noncopyable {
    public:
    explicit parse_context(pipeline_config &pc) : output_(pc) {
    }

    error_reporter &reporter() { return reporter_; }
    pipeline_config &output() { return output_; }

    //! \name Parser actions
    //! These maintain state within a parse.
    //@{

    void new_job() { current.job = &(output_.new_job()); }
    void new_section() { current.section = &NERVE_CHECK_PTR(current.job)->new_section(); }
    void new_stage() { current.stage = &NERVE_CHECK_PTR(current.section)->new_stage(); }

    void end_job() { current.reset(); }
    void end_section() { current.stage = NULL; current.section = NULL; }
    void end_stage() { current.stage = NULL; }

    stage_config &this_stage() { return *NERVE_CHECK_PTR(current.stage); }
    section_config &this_section() { return *NERVE_CHECK_PTR(current.section); }
    job_config &this_job() { return *NERVE_CHECK_PTR(current.job); }

    //@}

    private:
    pipeline_config &output_;

    // Differ depending on where we are in the parse.
    struct output_pointers {
      job_config *job;
      section_config *section;
      stage_config *stage;

      output_pointers() { reset(); }
      void reset() { job = NULL; section = NULL; stage = NULL; }
    } current;

    error_reporter reporter_;
  };
}
