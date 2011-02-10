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
#include "flex_interface.hpp"

#include "../plugin-proto/asserts.hpp"

#include <vector>
#include <list>
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

/*!
 * \ingroup grp_config
 *
 * Note that we don't configure sequences because that's implied by the type of
 * the stage.
 *
 * There are two ways of iterating the configs:
 *
 * - iterate jobs, iterate all sections (via the pipeline_next/prev) but skip
 *   those sections which aren't in the job (via. parent_jov)
 * - iterate jobs, iterate sections in that job (via. thread_next/prev)
 *
 * I think that the actual overhead is fairly similar for both.  The job
 * iteration and the job skipping is two comparisons and the section iteration
 * is one null-comparison but the number of sections is always large.
 *
 * The other method has one comparison for the job iteration and one
 * null-comparison for the section iteration, but has two additional null
 * comparisons to assign the in-job order list.
 *
 * Both methods are present because I don't really know which one will stand the
 * test of time.
 */
class section_config {
  public:
  typedef std::list<stage_config> stages_type;
  typedef stages_type::iterator stage_iterator_type;

  section_config()
  : pipeline_next_(NULL), pipeline_previous_(NULL),
    job_next_(NULL),
    parent_job_(NULL) {}

  stage_config &new_stage() {
    stages_.push_back(stage_config());
    return stages_.back();
  }

  void name(flex_interface::text_ptr pt) { name_ = pt; }
  void next_name(flex_interface::text_ptr pt, const parse_location &copy) {
    this->location_next(copy);
    next_name_ = pt;
  }

  bool job_last() const { return pipeline_next() == NULL; }
  bool job_first() const { return pipeline_previous() == NULL; }

  const char *name() const { return name_.get(); }
  const char *next_name() const { return next_name_.get(); }

  job_config &parent_job() { return *NERVE_CHECK_PTR(parent_job_); }
  void parent_job(job_config *p) { parent_job_ = NERVE_CHECK_PTR(p); }

  //! \name Ordering
  //!
  //! These specify null-terminated linked lists across sections.  Thread order
  //! is the list of sections with the same parent_job, while pipeline is the
  //! complete list of sections across the entire pipeline.
  //@{
  void pipeline_next(section_config *s) { pipeline_next_ = NERVE_CHECK_PTR(s); }
  void pipeline_previous(section_config *s) { pipeline_previous_ = NERVE_CHECK_PTR(s); }
  void job_next(section_config *s) { job_next_ = NERVE_CHECK_PTR(s); }
  section_config *pipeline_next() const { return pipeline_next_; }
  section_config *pipeline_previous() const { return pipeline_previous_; }
  section_config *job_next() const { return job_next_; }
  //@}

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

  section_config *pipeline_next_;
  section_config *pipeline_previous_;
  section_config *job_next_;

  job_config *parent_job_;
};

//! \ingroup grp_config
class job_config {
  public:

  typedef std::list<section_config> sections_type;
  typedef sections_type::iterator section_iterator_type;

  job_config()
  : job_first_(NULL),
    job_last_(NULL) {}

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

  //! The first section in this thread in pipeline order.
  void job_first(section_config *s) { job_first_ = NERVE_CHECK_PTR(s); }
  section_config *job_first()  { return job_first_; }

  //! This is used entirely for controlling the list linkage.
  void job_last(section_config *s) { job_last_ = NERVE_CHECK_PTR(s); }
  section_config *job_last()  { return job_last_; }

  private:
  sections_type sections_;
  section_config *job_first_;
  section_config *job_last_;
};

//! \ingroup grp_config
class pipeline_config {
  public:
  typedef std::list<job_config> jobs_type;
  typedef jobs_type::iterator job_iterator_type;

  pipeline_config()
  : pipeline_first_(NULL) {}

  //! Start a new job.
  job_config &new_job() {
    //TODO:
    //  This kind of thing only works because I'm using a list.  The pointers
    //  would be invalidated otherwise.  It would be a lot neater to store a
    //  vector of pointers to jobs.  Same goes for the rest.
    jobs_.push_back(job_config());
    return jobs_.back();
  }

  jobs_type &jobs() { return jobs_; }

  job_iterator_type begin() { return jobs().begin(); }
  job_iterator_type end() { return jobs().end(); }

  //! The globally first section (as opposed to the per-thread first).
  void pipeline_first(section_config *s) { pipeline_first_ = NERVE_CHECK_PTR(s); }
  section_config *pipeline_first() { return pipeline_first_; }

  //! Is there only a single section
  bool mono_section() const { return jobs_.size() == 1 && jobs_.begin()->mono_section(); }

  private:
  section_config *pipeline_first_;
  jobs_type jobs_;
};


} // ns config

#endif
