// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SEMANTIC_CHECKER_HPP_isu3fngy
#define PIPELINE_SEMANTIC_CHECKER_HPP_isu3fngy

#include "pipeline_configs.hpp"
#include "parse_context.hpp"

#include "../util/pooled.hpp"
#include "../util/c_string.hpp"

namespace config {
  struct error_reporter;

  /*!
   * \ingroup grp_config
   *
   * Stores the state for the semantic pass and lets us avoid having one big
   * nested loop uberalgorithm.  This is used exclusively by the config_parser.
   * Its purpose is to establish the section orderings and check the stage
   * orderings.
   */
  struct semantic_checker {
    // Quick way to track which names are declared and what jobs they're in.
    struct section_data {
      config::job_config     *parent;
      config::section_config *section;
    };

    typedef pooled::assoc<c_string, section_data>::map map_type;
    typedef map_type::key_type string_type;

    typedef config::pipeline_config::job_iterator_type   job_iter_t;
    typedef config::job_config::section_iterator_type    section_iter_t;
    typedef config::section_config::stage_iterator_type  stage_iter_t;

    typedef config::section_config section_config;
    typedef config::job_config     job_config;
    typedef config::stage_config   stage_config;

    semantic_checker(config::parse_context &ctx)
    : rep(ctx.reporter()),
      confs(ctx.output()),
      no_next(NULL),
      no_prev(NULL)
    {}

    //! Make a lookup table for section names.
    void register_names();

    //! The pipeline_next/prev bits in sections
    void link_pipeline_order();

    /*!
     * Sets up sections' job ordering, validates stage type ordering, and
     * assigns the "configure" blocks to a stage.  Linking jobs can't be done
     * until we have the entire pipeline order established because the section
     * order is not random-access comparable.  Iow, to compare to sections'
     * order, you have to iterate the whole pipeline.
     */
    void link_jobs_and_check_stage_order();

    private:

    //! If false then don't validate more of the section after.
    bool check_section_next_name(section_config *const sec);

    //! Establish a linked list based on the next_name and the name lookup table.
    void link_sections(job_config *const job, section_config *const sec);

    //! Checks stages are sorted in proper category order.
    void check_stage_order_visit(job_config *, section_config *, stage_config *const stage);

    //! Check that a stage is process or output and report if not.
    void check_process_or_output(const char *what);
    //! Check that a stage is observe and report error.
    void check_only_observe(const char *what);
    //! Error report for the above two.
    void report_bad_order(const char *format, const char *what);

    //! The configure "x { .. }" block assignment.
    void assign_configure_block(stage_config &st);

    config::error_reporter &rep;
    config::pipeline_config &confs;
    config::section_config *no_next;
    config::section_config *no_prev;

    map_type names;

    struct stage_visit_state {
      stage_visit_state() {
        initial = true;
        last_stage = NULL;
        this_stage = NULL;
      }

      bool initial;
      stage_config *last_stage;
      stage_config *this_stage;
    };

    stage_visit_state visit_state;
  };
}
#endif
