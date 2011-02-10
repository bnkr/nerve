// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Classes which store the configuration of the pipeline.
 *
 * These objects are pure data storage and are handled by the parsing state.
 * This means they are always mutable.
 */

#ifndef CONFIG_PIPELINE_CONFIGS_HPP_n8dsnrmy
#define CONFIG_PIPELINE_CONFIGS_HPP_n8dsnrmy

#include "parse_location.hpp"

#include <vector>
#include <cstring>

namespace config {

//! \ingroup grp_config
class stage_config {
  public:
  enum stage_ids {
    id_unset,
    id_plugin,
    id_sdl,
    id_ffmpeg
  };

  stage_config() : type_(id_unset) {}

  void path(const flex_interface::text_ptr &pt) { type(id_plugin); path_ = pt; }
  void type(enum stage_ids i) { type_ = i; }

  const char *path() const { return path_.get(); }
  stage_ids type() const { return type_; }

  //! Finds an identifier for the stage, returning id_plugin if it's a loadable
  //! plugin, id_unset if it can't be found, or the id of a built-in stage.
  static stage_ids find_stage(const char *name) {
    NERVE_ASSERT_PTR(name);

    if (std::strcmp(name, "sdl") == 0) {
      return stage_config::id_sdl;
    }
    else if (std::strcmp(name, "ffmpeg") == 0) {
      return stage_config::id_ffmpeg;
    }
    else {
      return stage_config::id_unset;
    }
  }

  private:
  flex_interface::text_ptr path_;
  stage_ids type_;
};

struct job_config;

//! \ingroup grp_config
//!
//! Note that we don't configure sequences because that's implied by the type of
//! the stage.
class section_config {
  public:
  typedef std::vector<stage_config> stages_type;
  typedef stages_type::iterator stage_iterator_type;

  section_config() : next_section_(NULL), previous_section_(NULL), parent_job_(NULL) {}

  stage_config &new_stage() {
    stages_.push_back(stage_config());
    return stages_.back();
  }

  void name(flex_interface::text_ptr pt) { name_ = pt; }
  void next_name(flex_interface::text_ptr pt, const parse_location &copy) {
    this->location_next(copy);
    next_name_ = pt;
  }

  bool last_section() const { return next_section() == NULL; }
  bool first_section() const { return previous_section() == NULL; }

  const char *name() const { return name_.get(); }
  const char *next_name() const { return next_name_.get(); }

  void next_section(section_config *s) { next_section_ = NERVE_CHECK_PTR(s); }
  void previous_section(section_config *s) { previous_section_ = NERVE_CHECK_PTR(s); }

  job_config &parent_job() { return *NERVE_CHECK_PTR(parent_job_); }
  void parent_job(job_config *p) { parent_job_ = NERVE_CHECK_PTR(p); }

  section_config *next_section() const { return next_section_; }
  section_config *previous_section() const { return previous_section_; }

  //! The place where "next" was given.  This is necesasry because the semantic
  //! pass should report errors in a sensible place, not the end of the
  //! document.
  const parse_location &location_next() { return location_next_; }
  void location_next(const parse_location &copy) { location_next_ = copy; }

  //! Location of the start of the section.
  const parse_location &location_start() { return location_start_; }
  void location_start(const parse_location &copy) { location_start_ = copy; }

  stages_type &stages() { return stages_; }

  stage_iterator_type begin() { return stages_.begin(); }
  stage_iterator_type end() { return stages_.end(); }

  private:
  stages_type stages_;
  flex_interface::text_ptr name_;
  flex_interface::text_ptr next_name_;
  parse_location location_next_;
  parse_location location_start_;

  section_config *next_section_;
  section_config *previous_section_;
  job_config *parent_job_;
};

//! \ingroup grp_config
class job_config {
  public:

  typedef std::vector<section_config> sections_type;
  typedef sections_type::iterator section_iterator_type;

  job_config() {}

  section_config &new_section() {
    sections_.push_back(section_config());
    return sections_.back();
  }

  bool empty() const { return sections_.empty(); }
  //! Is there precisely one section.
  bool mono_section() const { return sections_.size() == 1; }

  sections_type &sections() { return sections_; }

  section_iterator_type begin() { return sections_.begin(); }
  section_iterator_type end() { return sections_.end(); }

  private:
  sections_type sections_;
};

//! \ingroup grp_config
class pipeline_config {
  public:
  typedef std::vector<job_config> jobs_type;
  typedef jobs_type::iterator job_iterator_type;

  pipeline_config() : first_section_(NULL) {}

  //! Start a new job.
  job_config &new_job() {
    jobs_.push_back(job_config());
    return jobs_.back();
  }

  jobs_type &jobs() { return jobs_; }

  job_iterator_type begin() { return jobs().begin(); }
  job_iterator_type end() { return jobs().end(); }

  //! The first section in pileline order.  The full order is implied by the
  //! section prev/next links.  The order within a job is determined by the
  //! job() member of sections.
  void first_section(section_config *s) { first_section_ = NERVE_CHECK_PTR(s); }
  section_config &first_section()  { return *NERVE_CHECK_PTR(first_section_); }

  //! Is there only a single section
  bool mono_section() const { return jobs_.size() == 1 && jobs_[0].mono_section(); }

  private:
  section_config *first_section_;
  jobs_type jobs_;
};


} // ns config

#endif
