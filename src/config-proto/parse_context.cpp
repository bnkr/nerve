// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "flex_interface.hpp" // must come first
#include "parse_context.hpp"
#include "pipeline_configs.hpp"

using config::parse_context;

void parse_context::new_job() {
  current.reset();
  current.job = NERVE_CHECK_PTR(output_.new_job());
}

void parse_context::new_section() {
  job_config *const j = NERVE_CHECK_PTR(current.job);
  section_config &s = *NERVE_CHECK_PTR(j->new_section());
  s.parent_job(j);
  s.location_start(this->current_location());
  current.section = &s;
  current.stage = NULL;
}

void parse_context::new_stage() {
  current.stage = NERVE_CHECK_PTR(current.section)->new_stage();
  this_stage().location(this->current_location());
}

void parse_context::end_job() {
  if (this_job().empty()) {
    this->reporter().report("thread contains no sections");
  }
  current.reset();
}

void parse_context::end_section() {
  current.stage = NULL;
  current.section = NULL;
}

void parse_context::end_stage() {
  current.stage = NULL;
}

void parse_context::new_configure_block(char *name) {
  NERVE_ASSERT(current.configure == NULL, "shouldn't start a new configure until the onld one is done");

  config::pipeline_config::configure_blocks_type &blks = output_.configure_blocks();
  config::flex_interface::transfer_mem ptr(name);
  current.configure = NERVE_CHECK_PTR(blks.new_configure_block(ptr));
}

void parse_context::end_configure_block() {
  current.configure = NULL;
}

namespace {
  // Exception safety just in case.
  struct scoped_new_stage {
    scoped_new_stage(parse_context &pc) : pc_(pc) {
      pc_.new_stage();
    }

    ~scoped_new_stage() {
      pc_.end_stage();
    }

    parse_context &pc_;
  };
}

void parse_context::add_stage(char *text) {
  flex_interface::transfer_mem p(text);
  scoped_new_stage s(*this);
  typedef stage_config::plugin_id_type id_type;
  id_type id = stage_config::get_plugin_id(text);
  switch (id) {
  case stage_config::id_unset:
    reporter().report("not a valid stage id or not a loadable plugin: %s", text);
    break;
  case stage_config::id_plugin:
    this_stage().path(p);
    this_stage().plugin_id(id);
    break;
  default:
    this_stage().plugin_id(id);
    break;
  }
}
