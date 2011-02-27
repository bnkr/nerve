// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "semantic_checker.hpp"

#include "../util/asserts.hpp"

using namespace config;

/***************
 * Preparation *
 ***************/

void semantic_checker::register_names() {
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

/************
 * Ordering *
 ************/

void semantic_checker::link_pipeline_order() {
  for (job_iter_t job = confs.begin(); job != confs.end(); ++job) {
    NERVE_ASSERT(! job->sections().empty(), "there must always be sections in a job");
    job_config *const jp = &(*job);
    for (section_iter_t sec = job->begin(); sec != job->end(); ++sec) {
      NERVE_ASSERT(! sec->stages().empty(), "there must always be stages in a section");

      section_config *const sp = &(*sec);
      if (check_section_next_name(sp)) {
        link_sections(jp, sp);

        // Needed later to find the first section in pipeline order.
        if (sec->pipeline_previous() == NULL) {
          // there can be something in no_prev under error conditions.
          this->no_prev = sp;
        }
      }
    }
  }

  // We might not have visited it because it doesn't have to have a name.
  if (confs.mono_section()) {
    this->no_prev = &(*(confs.begin()->begin()));
  }

  NERVE_ASSERT(this->no_prev != NULL || rep.error(), "no_prev must have been assigned unless there was an erro");

  if (! this->no_prev) {
    return;
  }

  confs.pipeline_first(this->no_prev);
}

void semantic_checker::link_jobs_and_check_stage_order() {
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

    // Kill two birds and visit these in order.  Perhaps this could be done in
    // the other loop, but it mens redesigning the stage checking all because
    // we only know the "next section" pointer in that one.  This loop must be
    // done anyway.
    for (stage_iter_t stage = s->begin(); stage != s->end(); ++stage) {
      check_stage_order_visit(&(s->parent_job()), s, &(*stage));
      assign_configure_block(*stage);
    }
  } while ((s = s->pipeline_next()) != NULL);

  const stage_config::category_type last_cat = NERVE_CHECK_PTR(visit_state.last_stage)->category();
  const bool end_on_output = last_cat == stage_cat::output;
  const bool end_on_observe = last_cat == stage_cat::observe;

  // ending on observe implies we hit an output at some point
  if (! (end_on_output || end_on_observe)) {
    rep.report("no output stage present");
  }
}

/************
 * Sections *
 ************/

bool semantic_checker::check_section_next_name(section_config *const sec) {
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

void semantic_checker::link_sections(job_config *const job, section_config *const sec) {
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

/**********
 * Stages *
 **********/

void semantic_checker::check_stage_order_visit(job_config *, section_config *, stage_config *const stage) {
  if (visit_state.initial) {
    if (stage->category() != stage_cat::input) {
      rep.lreport(
        stage->location(), "first stage must be an input stage but its category is %s",
        stage->category_name()
      );
    }
    visit_state.initial = false;
    visit_state.last_stage = stage;
  }
  else {
    visit_state.this_stage = stage;

    switch (NERVE_CHECK_PTR(visit_state.last_stage)->category()) {
    case stage_cat::input:
      check_process_or_output("an input");
      break;
    case stage_cat::process:
      check_process_or_output("a process");
      break;
    case stage_cat::output:
      check_only_observe("an output");
      break;
    case stage_cat::observe:
      check_only_observe("an observe");
      break;
    case stage_cat::unset:
      NERVE_ABORT("category should not be unset at this point");
      break;
    }
  }

  visit_state.this_stage = NULL;
  visit_state.last_stage = stage;
}

/**************************
 * Stage ordering Utility *
 **************************/

void semantic_checker::check_process_or_output(const char *what) {
  switch (visit_state.this_stage->category()) {
  case stage_cat::output:
  case stage_cat::process:
    break;
  default:
    report_bad_order(
      "only output and process stages may be directly after %s stage", what
    );
  }
}

void semantic_checker::check_only_observe(const char *what) {
  if (visit_state.this_stage->category() != stage_cat::observe) {
    report_bad_order("only observe stages may be directly after %s stage", what);
  }
}

void semantic_checker::report_bad_order(const char *format, const char *what) {
  rep.lreport(visit_state.this_stage->location(), format, what);
  rep.lreport(
    visit_state.last_stage->location(), "the %s stage ordered before is here",
    visit_state.last_stage->category_name()
  );
}

void semantic_checker::assign_configure_block(stage_config &st) {
  typedef config::pipeline_config::configure_blocks_type blocks_type;

  blocks_type &blocks = confs.configure_blocks();

  // TODO:
  //   This doesn't work when there are multiple occurances of the same stage
  //   and we want to configure them differently.  See also note in
  //   parse_context's block adding bit.  -- Bunker, 12 Feb.

  if (blocks.has(NERVE_CHECK_PTR(st.name()))) {
    blocks_type::block_type *const block = blocks.get(st.name());
    st.configs(block);
  }
}
