// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SECTION_HPP_1auycg4v
#define PIPELINE_SECTION_HPP_1auycg4v

#include "stage_sequence.hpp"
#include "stage_data.hpp"
#include "connectors.hpp"

#include "../util/indirect.hpp"
#include <boost/type_traits/remove_pointer.hpp>

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * The section class contains multiple types of stages via the mono-typed
   * sequence containers and always communicates with other threads (excepting the
   * input stage which might not).
   *
   * The section's purpose is as follows:
   *
   * - defines direct connection boundaries, i.e sections always talk to other
   *   threads but sequences don't.  This is necessary to know or you can't
   *   guarentee one wait per job iteration (which itself is necessary to
   *   prevent deadlocks).
   * - allows sequences to be specialised to each category of stage which is
   *   necessary to avoid extra work, e.g progressive buffering on observer
   *   stages.  It also means we can have a much tidier stage API.
   *
   * This implementation adds some overhead and complication, but it does appear
   * to be the best solution.
   *
   * Communication between stages is abstract.  An alternative implementation
   * method is to handle the connections (just the thread pipes) in this object
   * and have direct returns (using packet_return) which is then put on the
   * section's pipe.  This is not done for the following reasons:
   *
   * - thread pipe operations (such as the clear on abandon) couldn't be
   *   enforced unless the event type is checked again here
   * - if sequences give us their output packet then we nececssarily need to
   *   handle their input packets too.  This means we need to deal with
   *   progressive buffering which obviates specialised sequences (in effect it
   *   moves the problem that the current section class solves up one level --
   *   we'd need specialised per-stage sections)
   *
   * The result is that for every sequence there is a virtual call to write
   * output, but but we don't need to do a buffering/no-return check or event
   * check.  Furthermore, given that each sequence is polymorphic anyway, it
   * might be that we can optimise out the polymorphic connector.
   */
  class section {
    public:

    //! Must be separate because the sequence is polymorphic.
    struct sequence_destructor {
      // Storing the number of bytes allocated doesn't actually increase our
      // overhead.  If we didn't store it then we'd need to store some kind of
      // run-time type information anyway (e.g the stage_category_type)
      void operator()(stage_sequence *const ss) { pooled::tracked_byte_free(ss); }
    };

    typedef pooled::container<stage_sequence*>::vector vector_type;
    typedef indirect_owned<vector_type, sequence_destructor> sequences_type;

    typedef boost::remove_pointer<sequences_type::value_type>::type sequence_type;

    typedef stage_category_type category_type;

    //! \name Initialisation etc.
    //@{

    explicit section() {}

    connector *input_pipe() { return in_pipe_; }
    connector *output_pipe() { return out_pipe_; }

    void input_pipe(connector *i) { in_pipe_ = i; }
    void output_pipe(connector *o) { out_pipe_ = o; }

    //! Create a new sequence in this section which is valid for the given
    //! stage category.  Pointers must remain valid.
    stage_sequence *create_sequence(category_type t);

    //! Called post-initialisation to ready everything for running.  Iow no more
    //! sequences to add.
    void finalise();

    //@}

    //! \name Operation
    //@{

    bool would_block() {
      return
        ! buffering_sequence() &&
        // It's only necessary to check the input because we can assume that
        // something is waiting for the output (or will be eventually) and won't
        // be in the same thread.
        //
        // TODO:
        //   The above might be wrong.
        //
        // TODO:
        //   Unspecified for now because it depends on connectors.
        // NERVE_CHECK_PTR(*start())->input_connector().would_block()
        false;
    }

    //! Operational part.
    void section_step();

    //@}

    private:
    sequences_type &sequences() { return sequences_; }

    typedef sequences_type::iterator iterator_type;

    bool buffering_sequence() { return start() == sequences().begin(); }

    void reset_start() { start_ = sequences().begin(); }
    void start(iterator_type i) { start_ = i; }
    iterator_type start() { return start_; }

    iterator_type start_;
    sequences_type sequences_;

    connector *in_pipe_;
    connector *out_pipe_;
  };
}
#endif
