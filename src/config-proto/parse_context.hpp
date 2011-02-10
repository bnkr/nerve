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

    const error_reporter &reporter() const { return reporter_; }
    error_reporter &reporter() { return reporter_; }
    pipeline_config &output() { return output_; }

    const parse_location &current_location() const { return this->reporter().location(); }

    //! \name Parser actions
    //! These maintain state within a parse.
    //@{

    void new_job() { current.job = &(output_.new_job()); }

    void new_section() {
      job_config &j = *NERVE_CHECK_PTR(current.job);
      section_config &s = j.new_section();
      s.parent_job(&j);
      s.location_start(this->current_location());
      current.section = &s;
      current.stage = NULL;
    }

    void new_stage() { current.stage = &NERVE_CHECK_PTR(current.section)->new_stage(); }

    void end_job() {
      if (this_job().empty()) {
        error_reporter().report("thread contains no sections");
      }
      current.reset();
    }

    void end_section() { current.stage = NULL; current.section = NULL; }
    void end_stage() { current.stage = NULL; }

    stage_config &this_stage() { return *NERVE_CHECK_PTR(current.stage); }
    section_config &this_section() { return *NERVE_CHECK_PTR(current.section); }
    job_config &this_job() { return *NERVE_CHECK_PTR(current.job); }

    //@}

    //! \name Stage Actions
    //! Complete stage config creation.
    //@{

    typedef flex_interface::text_ptr text_ptr;

    void add_stage(text_ptr text) {
      new_stage();
      typedef stage_config::stage_ids id_type;
      id_type id = stage_config::find_stage(text.get());
      switch (id) {
      case stage_config::id_unset:
        reporter().report("not a valid stage id or not a loadable plugin: %s", text.get());
        break;
      case stage_config::id_plugin:
        this_stage().path(text);
        this_stage().type(id);
        break;
      default:
        this_stage().type(id);
        break;
      }
      end_stage();
    }

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
