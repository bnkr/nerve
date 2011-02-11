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
#include "pooled.hpp"

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/utility.hpp>
#include <cstring>

namespace config {
  template<class Iterator>
  typename boost::indirect_iterator<Iterator> make_deref_iter(Iterator i) {
    return typename boost::indirect_iterator<Iterator>(i);
  }

//! \ingroup grp_config
class stage_config : boost::noncopyable {
  public:
  enum stage_ids {
    id_unset,
    id_plugin,
    id_sdl,
    id_ffmpeg
  };

  enum categories {
    // Note that a separate type is necessary for the output despite it
    // essentially being an observer because we need to know that one exists in
    // the pipeline.
    cat_output,
    cat_input,
    cat_process,
    cat_observe
  };

  typedef stage_config * create_type;
  static create_type create() {
    return pooled::alloc<stage_config>();
  }

  static void destroy(create_type p) {
    pooled::free(p);
  }

  stage_config() : type_(id_unset) {}

  //! The name of this stage's plugin (e.g sdl)
  const char *name() const {
    switch (this->type()) {
    case id_unset:
      NERVE_ABORT("don't call this until the path/id is done");
    case id_plugin:
      return NERVE_CHECK_PTR(this->path());
    case id_sdl:
      return "sdl";
    case id_ffmpeg:
      return "ffmpeg";
    }

    NERVE_ABORT("what are you doing here?");
  }

  //! What kind of stage it is (e.g process)
  const char *category_name() const {
    return get_category_name(this->category());
  }

  inline static const char *get_category_name(categories c) {
    switch (c) {
    case cat_input:
      return "input";
    case cat_output:
      return "output";
    case cat_process:
      return "process";
    case cat_observe:
      return "observe";
    }

    NERVE_ABORT("what are you doing here?");
  }

  categories category() const {
    switch (this->type()) {
    case id_unset:
      NERVE_ABORT("don't call this until the path/id is done");
    case id_plugin:
      NERVE_ABORT("not implemented: finding the category from a plugin path");
    case id_ffmpeg:
      return cat_input;
    case id_sdl:
      return cat_output;
    }

    NERVE_ABORT("what are you doing here?");
  }

  void path(const flex_interface::text_ptr &pt) { type(id_plugin); path_ = pt; }
  void type(enum stage_ids i) { type_ = i; }

  //! Is the stage to be loaded from a shared object.
  bool file_based() const {
    // TODO:
    //   This kind of thing really indicates we need another object to put this
    //   one in which can only be constructed when it's ready to be queried.
    NERVE_ASSERT(type() != id_unset, "not initialised yet");
    return type() == id_plugin;
  }

  const char *path() const { return path_.get(); }
  stage_ids type() const { return type_; }

  //! Place this was declared.
  const parse_location &location() const { return location_; }
  void location(const parse_location &copy) { location_ = copy; }

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
  parse_location location_;
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
class section_config : boost::noncopyable {
  public:
  typedef section_config *create_type;
  // This could be a ptr_vector which would give us nicer functions (it
  // automatically dereferences) but it's suboptimal when you have total
  // ownership of the pointer.
  typedef pooled::container<stage_config::create_type>::vector stages_type;
  typedef boost::indirect_iterator<stages_type::iterator> stage_iterator_type;

  //! \name Resources
  //@{

  section_config()
  : pipeline_next_(NULL), pipeline_previous_(NULL),
    job_next_(NULL),
    parent_job_(NULL) {}

  ~section_config() {
    std::for_each(stages().begin(), stages().end(), stage_config::destroy);
  }

  stage_config *new_stage() {
    stages_.push_back(stage_config::create());
    return stages_.back();
  }

  typedef boost::fast_pool_allocator<section_config> allocator_type;
  static allocator_type section_alloc;

  static create_type create() {
    return pooled::alloc<section_config>();
  }

  static void destroy(create_type p) {
    return pooled::free(p);
  }

  //@}

  //! \name Attributes
  //@{

  stages_type &stages() { return stages_; }
  stage_iterator_type begin() { return make_deref_iter(stages_.begin()); }
  stage_iterator_type end() { return make_deref_iter(stages_.end()); }

  void name(flex_interface::text_ptr pt) { name_ = pt; }
  //! Will be looked up in the semantic pass.
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

  //@}

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

  //! \name Locations
  //@{

  //! The place where "next" was given.  This is necesasry because the semantic
  //! pass should report errors in a sensible place, not the end of the
  //! document.
  const parse_location &location_next() { return location_next_; }
  void location_next(const parse_location &copy) { location_next_ = copy; }

  //! Location of the start of the section.
  const parse_location &location_start() { return location_start_; }
  void location_start(const parse_location &copy) { location_start_ = copy; }

  //@}

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
class job_config : boost::noncopyable {
  public:
  typedef job_config * create_type;
  typedef pooled::container<section_config::create_type>::vector sections_type;
  typedef boost::indirect_iterator<sections_type::iterator> section_iterator_type;

  static create_type create() {
    return pooled::alloc<job_config>();
  }

  static void destroy(create_type p) {
    pooled::free<job_config>(p);
  }

  job_config()
  : job_first_(NULL),
    job_last_(NULL) {}

  ~job_config() {
    std::for_each(sections().begin(), sections().end(), section_config::destroy);
  }

  section_config *new_section() {
    sections_.push_back(NERVE_CHECK_PTR(section_config::create()));
    return sections_.back();
  }

  bool empty() const { return sections_.empty(); }
  //! Is there precisely one section.
  bool mono_section() const { return sections_.size() == 1; }

  sections_type &sections() { return sections_; }

  section_iterator_type begin() { return make_deref_iter(sections_.begin()); }
  section_iterator_type end() { return make_deref_iter(sections_.end()); }

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
class pipeline_config : boost::noncopyable {
  public:
  typedef pooled::container<job_config::create_type>::vector jobs_type;
  typedef boost::indirect_iterator<jobs_type::iterator> job_iterator_type;

  pipeline_config()
  : pipeline_first_(NULL) { jobs().reserve(16); }

  ~pipeline_config() {
    std::for_each(jobs().begin(), jobs().end(), job_config::destroy);
  }

  //! Start a new job.
  job_config *new_job() {
    jobs_.push_back(job_config::create());
    return jobs_.back();
  }

  jobs_type &jobs() { return jobs_; }
  const jobs_type &jobs() const { return jobs_; }

  job_iterator_type begin() { return make_deref_iter(jobs().begin()); }
  job_iterator_type end() { return make_deref_iter(jobs().end()); }

  //! The globally first section (as opposed to the per-thread first).
  void pipeline_first(section_config *s) { pipeline_first_ = NERVE_CHECK_PTR(s); }
  section_config *pipeline_first() { return pipeline_first_; }

  //! Is there only a single section
  bool mono_section() const { return jobs().size() == 1 && make_deref_iter(jobs_.begin())->mono_section(); }

  private:
  section_config *pipeline_first_;
  jobs_type jobs_;
};


} // ns config

#endif
