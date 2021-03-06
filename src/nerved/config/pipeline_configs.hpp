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
#include "../stages/stage_data.hpp"

#include "../util/c_string.hpp"
#include "../util/asserts.hpp"
#include "../util/pooled.hpp"

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <algorithm>

namespace pipeline { class job; }

// TODO:
//   This lot should use the indirect_owned container thing.

namespace config {
  template<class Iterator>
  typename boost::indirect_iterator<Iterator> make_deref_iter(Iterator i) {
    return typename boost::indirect_iterator<Iterator>(i);
  }

  struct configure_block;

  //! \ingroup grp_config
  //! Alias for stage categories.
  namespace stage_cat = ::stages::stage_cat;
  //! \ingroup grp_config
  //! Alias for stage built-in stages which we'll use to decide whether to load
  //! from a file or from built-in.
  namespace plug_id = ::stages::plug_id;

  //! \ingroup grp_config
  class stage_config : boost::noncopyable {
    public:
    typedef stages::stage_data stage_data_type;
    typedef stage_data_type::plugin_id_type plugin_id_type;

    typedef ::stages::category_type category_type;

    typedef stage_config * create_type;

    typedef flex_interface::unique_ptr unique_text_ptr;
    typedef flex_interface::transfer_mem transfer_mem;
    typedef configure_block configs_type;

    //! \name Construct/destruct
    //@{

    static create_type create() { return pooled::alloc<stage_config>(); }
    static void destroy(create_type p) { pooled::free(p); }

    stage_config() : configs_(NULL) {}

    //@}

    //! \name Enum to text
    //@{

    static const char *get_stage_name(const stage_config &c);

    //@}

    //! The name of this stage's plugin (e.g sdl).  This takes loadable plugins
    //! into account as well as internal ones.
    const char *name() const { return get_stage_name(*this); }

    //! What kind of stage it is (e.g process)
    const char *category_name() const { return stages::get_category_name(this->category()); }

    //! Stage concept (e.g observer).
    category_type category() const;

    //! Location to load from (only when type == id_plugin)
    const char *path() const { return path_.get(); }
    void path(transfer_mem &pt) { path_ = pt.release_exclusive(); }

    //! Numeric type for the internal stage or id_plugin for check path.
    void plugin_id(plugin_id_type i) { data_.plugin_id(i); }
    plugin_id_type plugin_id() const { return data_.plugin_id(); }

    //! Is the stage built in?
    bool internal() const { return data_.built_in(); }

    //! Stage module data.
    const stage_data_type &stage_data() const { return data_; }
    stage_data_type &stage_data() { return data_; }

    //! Place this was declared.
    const parse_location &location() const { return location_; }
    void location(const parse_location &copy) { location_ = copy; }

    //! The key=value configurations applied to this stage.
    void configs(configs_type *c) { configs_ = c; }
    configs_type *configs() { return configs_; }
    bool configs_given() const { return configs_ != NULL; }

    private:
    stages::stage_data data_;
    unique_text_ptr path_;
    parse_location location_;
    configs_type *configs_;
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

  void name(flex_interface::transfer_mem &pt) { name_ = pt.release_exclusive(); }
  //! Will be looked up in the semantic pass.
  void next_name(flex_interface::transfer_mem &pt, const parse_location &copy) {
    this->location_next(copy);
    next_name_ = pt.release_exclusive();
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

  flex_interface::unique_ptr name_;
  flex_interface::unique_ptr next_name_;

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
    job_last_(NULL),
    configured_job_(NULL)
  {}

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

  //! Pointer to the pipeline object which has been created.  This is a bit of a
  //! hack to store it here but it means we don't have to allocate anything or
  //! do extra passes in order to configure by iterating over sections.  The
  //! section creation can test sec_conf.parent_job().configured_job().
  pipeline::job *configured_job() const { return configured_job_; }
  void configured_job(pipeline::job *v) { configured_job_ = v; }

  private:
  sections_type sections_;
  section_config *job_first_;
  section_config *job_last_;
  pipeline::job *configured_job_;
};

//! \ingroup grp_config
class configure_block : boost::noncopyable {
  public:

  typedef flex_interface::shared_ptr shared_ptr;
  typedef flex_interface::unique_ptr unique_ptr;
  typedef flex_interface::transfer_mem transfer_mem;
  // This must be shared because it's going in a container.
  typedef std::pair<shared_ptr, shared_ptr> ptr_pair_type;

  struct field_pair : protected ptr_pair_type {
    const char *field() const { return this->first.get(); }
    const char *value() const { return this->second.get(); }

    void field(shared_ptr p) { this->first = p; }
    void value(shared_ptr p) { this->second = p; }
  };

  typedef pooled::container<field_pair>::vector pairs_type;

  typedef configure_block * create_type;
  static create_type create() { return pooled::alloc<configure_block>(); }
  static void destroy(create_type p) { pooled::free(p); }

  const char *name() const  { return name_.get(); }
  void name(transfer_mem &p) { name_ = p.release_exclusive(); }
  void location(const parse_location &l) { location_ = l; }
  const parse_location &location() { return location_; }

  void new_pair(transfer_mem &field, transfer_mem &val) {
    pairs_.push_back(field_pair());
    pairs_.back().field(field.release_shared());
    pairs_.back().value(val.release_shared());
  }

  pairs_type &pairs() { return pairs_; }

  unique_ptr name_;
  pairs_type pairs_;
  parse_location location_;
};

//! \ingroup grp_config
//!
//! Container for the name { key-value } kind of config.
class configure_block_container : boost::noncopyable {
  public:

  // TODO:
  //   It might turn out that this design is too messy.  It certainly uses more
  //   memory than necesary because we don't actually need to hold the whole
  //   name -> [k,v] mapping after we have got all the stages in.  It depends
  //   how the stage creation happens so it's staying how it is for the moment.
  //
  //   The otherr reason it seems messy is that stages' "next" name is looked up
  //   usign state in the sematntic pass.  That state should be in the syntax
  //   pass (the parse context class prolly).  It would be consistent to do this
  //   mapping in the same place too a nd then chuck the name mappings away
  //   when we're done with them (ie straight before declareing pipeline
  //   objects).

  typedef flex_interface::transfer_mem transfer_mem;
  typedef transfer_mem::unique_type unique_text_ptr;
  typedef transfer_mem::shared_type shared_text_ptr;

  // TODO: This should prolly be a set since the string is in there anyway.
  typedef pooled::assoc<c_string, configure_block::create_type>::map blocks_type;
  typedef boost::remove_pointer<blocks_type::value_type::second_type>::type block_type;

  ~configure_block_container() { destroy_blocks(); }

  configure_block *new_configure_block(transfer_mem &name, const parse_location &copy) {
    configure_block::create_type block = configure_block::create();
    block->name(name);
    block->location(copy);
    blocks_[c_string(block->name())] = block;
    return block;
  }

  bool has(const char *name) const { return blocks().count(c_string(name)) > 0; }

  block_type *get(const char *name) {
    NERVE_ASSERT(this->has(name), "can't use field accessors if field doesn't exist");
    return blocks_[c_string(name)];
  }

  blocks_type &blocks() { return blocks_; }
  const blocks_type &blocks() const { return blocks_; }

  void clear() { destroy_blocks(); blocks().clear(); }

  private:
  void destroy_blocks();

  private:
  blocks_type blocks_;
};

//! \ingroup grp_config
class pipeline_config : boost::noncopyable {
  // TODO:
  //   This needs a better name because it's going to get various other bits in
  //   it too.  Perhaps I could get away with just encapsulating it somewhere
  //   else, though?
  public:
  typedef pooled::container<job_config::create_type>::vector jobs_type;
  typedef boost::indirect_iterator<jobs_type::iterator> job_iterator_type;
  typedef configure_block_container configure_blocks_type;

  pipeline_config()
  : pipeline_first_(NULL) { }

  ~pipeline_config() {
    std::for_each(jobs().begin(), jobs().end(), job_config::destroy);
  }

  //! Start a new job.
  job_config *new_job() {
    jobs_.push_back(job_config::create());
    return jobs_.back();
  }

  configure_blocks_type &configure_blocks() { return configure_blocks_; }

  jobs_type &jobs() { return jobs_; }
  const jobs_type &jobs() const { return jobs_; }

  job_iterator_type begin() { return make_deref_iter(jobs().begin()); }
  job_iterator_type end() { return make_deref_iter(jobs().end()); }

  //! The globally first section (as opposed to the per-thread first).
  void pipeline_first(section_config *s) { pipeline_first_ = NERVE_CHECK_PTR(s); }
  section_config *pipeline_first() { return pipeline_first_; }

  //! Is there only a single section
  bool mono_section() const {
    return
      jobs().size() == 1
      && make_deref_iter(jobs_.begin())->mono_section();
  }

  //! SHUT DOWN EVERYTHING!!!11
  void madagascar();
  void clear() { madagascar(); }

  private:
  section_config *pipeline_first_;
  configure_blocks_type configure_blocks_;
  jobs_type jobs_;
};


//! \ingroup grp_config
//! Outputs all the config stuff as basic yaml
void dump_config_yaml(config::pipeline_config &pc);

} // ns config

#endif
