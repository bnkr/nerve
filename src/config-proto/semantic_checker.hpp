// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SEMANTIC_CHECKER_HPP_isu3fngy
#define PIPELINE_SEMANTIC_CHECKER_HPP_isu3fngy

#include "pooled_stl.hpp"
#include "pipeline_configs.hpp"
#include "parse_context.hpp"

#include "../plugin-proto/asserts.hpp"

#include <cstring>

//! Something we can put in a map.
struct c_string {
  explicit c_string(const char *c) : str_(NERVE_CHECK_PTR(c)) {}

  bool operator<(const c_string &c) const { return std::strcmp(c.str_, this->str_) < 0; }

  const char *c_str() { return str_; }

  const char *str_;
};

//! Stores the state for the semantic and lets us avoid having one big nested
//! loop uberalgorithm.  This is used exclusively by the config_parser.
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

  void check() {
    register_names();
    traverse();
  }

  private:

  //! Make a lookup table for section names.
  void register_names() {
    // TODO:
    //   This could be done as we traverse the nodes.
    NERVE_ASSERT(! confs.jobs().empty(), "parser must not succeed if there are no job confs");

    for (job_iter_t job = confs.begin(); job != confs.end(); ++job) {
      NERVE_ASSERT(! job->sections().empty(), "parser must not succeed if there are no sections in a job");

      for (section_iter_t sec = job->begin(); sec != job->end(); ++sec) {
        string_type n(NERVE_CHECK_PTR(sec->name()));

        if (names.count(n)) {
          rep.lreport(sec->location_start(), "section name %s already used", n.c_str());
          config::section_config *const other_sec = NERVE_CHECK_PTR(names[n].section);
          rep.lreport(other_sec->location_start(), "name used here");
        }
        else {
          section_data &sd = names[n];
          sd.parent = &(*job);
          sd.section = &(*sec);
        }
      }
    }
  }

  //! Traverse the tree validating and semanticising each bit.
  void traverse() {
    for (job_iter_t job = confs.begin(); job != confs.end(); ++job) {
      NERVE_ASSERT(! job->sections().empty(), "there must always be sections in a job");
      for (section_iter_t sec = job->begin(); sec != job->end(); ++sec) {
        NERVE_ASSERT(! sec->stages().empty(), "there must always be stages in a section");
        visit_section(&(*job), &(*sec));
      }
    }

    // We might not have visited it because it doesn't have to have a name.
    if (confs.mono_section()) {
      this->no_prev = &(*(confs.begin()->sections().begin()));
    }

    NERVE_ASSERT(this->no_prev != NULL || rep.error(), "no_prev must have been assigned unless there was an erro");

    if (! this->no_prev) {
      return;
    }

    confs.pipeline_first(this->no_prev);

    section_config *s = NERVE_CHECK_PTR(confs.pipeline_first());
    do {
      // This might turn out to be unnecessary.  It gives us a way to iterate
      // sections based on a job rather than iterating every section and
      // checking the parent_job.
      job_config &j = s->parent_job();
      if (j.job_first() == NULL) {
        NERVE_ASSERT(j.job_last() == NULL, "last section is null if we haven't assigned a first section yet");
        j.job_first(s);
        j.job_last(s);
      }
      else {
        section_config *const l = NERVE_CHECK_PTR(j.job_last());
        l->job_next(s);
        j.job_last(s);
      }

      // Kill two birds and visit these in order.
      for (stage_iter_t stage = s->begin(); stage != s->end(); ++stage) {
        visit_stage(&(s->parent_job()), s, &(*stage));
      }
    } while ((s = s->pipeline_next()) != NULL);

    const bool end_on_output = visit_state.last_cat == stage_config::cat_output;
    const bool end_on_observe = visit_state.last_cat == stage_config::cat_observe;

    // ending on observe implies we hit an output at some point
    if (! (end_on_output || end_on_observe)) {
      rep.report("no output stage present");
    }
  }

  // Sections //

  void visit_section(job_config *const job, section_config *const sec) {
    if (check_section_next_name(sec)) {
      link_sections(job, sec);

      // Needed later to find the first section in pipeline order.
      if (sec->pipeline_previous() == NULL) {
        // there can be something in no_prev under error conditions.
        this->no_prev = sec;
      }
    }
  }

  //! If false then don't validate more of the section after.
  bool check_section_next_name(section_config *const sec) {
    if (sec->next_name() == NULL) {
      if (no_next != NULL) {
        rep.lreport(sec->location_start(), "only one section may have no 'next' (the output section)");
        rep.lreport(no_next->location_start(), "other section with no 'next' is here");
      }
      else {
        no_next = sec;
      }

      return false;
    }
    else {
      return true;
    }
  }

  //! Establish a linked list based on the next_name and the name lookup table.
  void link_sections(job_config *const job, section_config *const sec) {
    string_type next_name(NERVE_CHECK_PTR(sec->next_name()));
    if (names.count(next_name)) {
      section_data &data = names[next_name];

      section_config *const pipeline_next = NERVE_CHECK_PTR(data.section);
      section_config *const this_section = sec;

      job_config *const this_job = &(*job);
      job_config *const next_job = NERVE_CHECK_PTR(data.parent);

      if (this_job == next_job) {
        rep.lreport(
          this_section->location_start(),
          "section %s's next is %s which is in the same thread",
          NERVE_CHECK_PTR(this_section->name()), NERVE_CHECK_PTR(pipeline_next->name())
        );
        rep.lreport(
          pipeline_next->location_start(),
          "section %s is here",
          NERVE_CHECK_PTR(pipeline_next->name())
        );
      }

      pipeline_next->pipeline_previous(this_section);
      this_section->pipeline_next(pipeline_next);
    }
    else {
      rep.lreport(
        sec->location_next(),
        "section '%s' is after non-existent section '%s'",
        NERVE_CHECK_PTR(sec->name()), NERVE_CHECK_PTR(sec->next_name())
      );
    }
  }

  // Stages //

  struct visit_visit_state {
    visit_visit_state() : initial(true) {}

    bool initial;
    stage_config *last_stage;
    stage_config *this_stage;
    stage_config::categories last_cat;
  };

  void visit_stage(job_config *const job, section_config *const sec, stage_config *const stage) {
    if (visit_state.initial) {
      if (stage->category() != stage_config::cat_input) {
        rep.lreport(
          stage->location(), "first stage must be an input stage but its category is %s",
          stage->category_name()
        );
      }
      visit_state.initial = false;
    }
    else {
      visit_state.this_stage = stage;

      switch (visit_state.last_cat) {
      case stage_config::cat_input:
        check_process_or_output("an input");
      case stage_config::cat_process:
        check_process_or_output("a process");
      case stage_config::cat_output:
        check_only_observe("an output");
      case stage_config::cat_observe:
        check_only_observe("an observe");
      }
    }

    visit_state.this_stage = NULL;
    visit_state.last_cat = stage->category();
    visit_state.last_stage = stage;
  }

  void check_process_or_output(const char *what) {
    switch (visit_state.this_stage->category()) {
    case stage_config::cat_output:
    case stage_config::cat_process:
      break;
    default:
      report_bad_order(
        "only output and process stages may be directly after %s stage", what
      );
    }
  }

  void check_only_observe(const char *what) {
    if (visit_state.this_stage->category() != stage_config::cat_observe) {
      report_bad_order("only observe stages may be directly after %s stage", what);
    }
  }

  void report_bad_order(const char *format, const char *what) {
    rep.lreport(visit_state.this_stage->location(), format, what);
    rep.lreport(
      visit_state.last_stage->location(), "the %s stage ordered before is here",
      visit_state.last_stage->category_name()
    );
  }

  config::error_reporter &rep;
  config::pipeline_config &confs;
  config::section_config *no_next;
  config::section_config *no_prev;

  map_type names;

  visit_visit_state visit_state;
};
#endif
