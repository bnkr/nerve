// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef CONFIG_PARSE_CONTEXT_HPP_s4d4zw7e
#define CONFIG_PARSE_CONTEXT_HPP_s4d4zw7e

#include "error_reporter.hpp"

#include "../plugin-proto/asserts.hpp"

#include <boost/utility.hpp>

namespace config {
  class pipeline_config;
  class stage_config;
  class section_config;
  class job_config;

  //! \ingroup grp_config
  //! Parsing context used by the lexer and parser.
  class parse_context : boost::noncopyable {
    public:
    explicit parse_context(pipeline_config &pc)
    : output_(pc) {
    }

    ~parse_context() { flex_interface::destroy(); }

    //! \name Attributes
    //@{

    const error_reporter &reporter() const { return reporter_; }
    error_reporter &reporter() { return reporter_; }
    pipeline_config &output() { return output_; }

    const parse_location &current_location() const { return this->reporter().location(); }

    //@}

    //! \name Current targets
    //!
    //! Set up by the new_x functions, these are where we write data to.
    //@{

    stage_config &this_stage() { return *NERVE_CHECK_PTR(current.stage); }
    section_config &this_section() { return *NERVE_CHECK_PTR(current.section); }
    job_config &this_job() { return *NERVE_CHECK_PTR(current.job); }

    //@}

    //! \name Parser actions
    //! These maintain state within a parse.
    //@{

    void new_job();
    void new_section();
    void new_stage();
    void end_job();
    void end_section();
    void end_stage();

    //@}

    //! \name Stage Actions
    //! Complete stage config creation.
    //@{

    typedef flex_interface::text_ptr text_ptr;
    void add_stage(text_ptr name_or_path);

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

} // ns config
#endif
