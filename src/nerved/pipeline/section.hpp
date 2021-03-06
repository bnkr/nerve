// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SECTION_HPP_1auycg4v
#define PIPELINE_SECTION_HPP_1auycg4v

#include "stage_sequence.hpp"
#include "connection.hpp"
#include "thread_pipe.hpp"

#include "../stages/information.hpp"
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
   * section's thread_pipe.  This is not done for the following reasons:
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
   * check.  We can't optimise out the polymorphic pipe here because it can be a
   * terminator.
   */
  class section {
    public:
    typedef indirect_owned_polymorph<stage_sequence> sequences_type;
    typedef sequences_type::value_type sequence_type;

    //! \name Initialisation etc.
    //@{

    // Must be polymorphic because we can take the terminator as well.
    //
    // TODO:
    //   It's possible we could factor this out and have en entire sequence do
    //   the work of the terminators.  It's another performance issue where it's
    //   not clear which would be faster, because they both involve virtual
    //   functions...
    typedef polymorphic_connection connection_type;

    explicit section() : thread_pipe_allocated_(false) {}

    connection_type &connection() { return conn_; }

    //! Create a new sequence in this section which is valid for the given
    //! stage category.  Pointers must remain valid.
    stage_sequence *create_sequence(stages::category_type t, pipe *, pipe *);

    //! Stored locally to avoids allocation for the price of making this object
    //! waste space when it's one of the terminators.
    thread_pipe *create_thread_pipe() {
      NERVE_ASSERT(thread_pipe_allocated_ == false, "thread pipe may only be used once per section");
      thread_pipe_allocated_ = true;
      return &thread_pipe_;
    }

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
        //   Unspecified for now because it depends on pipes.
        // NERVE_CHECK_PTR(*start())->input_pipe().would_block()
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

    bool thread_pipe_allocated_;
    thread_pipe thread_pipe_;
    connection_type conn_;
  };
}
#endif
