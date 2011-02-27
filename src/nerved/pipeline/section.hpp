// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SECTION_HPP_1auycg4v
#define PIPELINE_SECTION_HPP_1auycg4v

#include "stage_sequence.hpp"
#include "stage_data.hpp"
#include "connectors.hpp"

#include <vector>
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
   *   moves the problem that the current section class solves up one level)
   *
   * The result is that for every sequence there is a virtual call to write
   * output, but but we don't need to do a buffering/no-return check or event
   * check.  Furthermore, given that each sequence is polymorphic anyway, it
   * might be that we can optimise out the polymorphic connector.
   */
  class section {
    public:
    typedef std::vector<stage_sequence*> sequences_type;
    typedef boost::remove_pointer<sequences_type::value_type>::type sequence_type;

    explicit section(sequences_type &seq) : sequences_(seq) {
      // TODO:
      //   This requirement might have to be lifted.  We'd have some kind of
      //   "prepare" method.  The stages might turn out to need this anyway, and
      //   it could help us do things like reconfigure at runtime.
      NERVE_ASSERT(! sequences().empty(), "sequence list must be initialised before this");
      reset_start();
    }

    // TODO:
    //   Ideally this should be in some special shared code between config and
    //   pipeline.
    typedef stage_category_type category_type;

    stage_sequence *create_sequence(category_type t);

    connector *output_pipe();
    connector *input_pipe();

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

    void section_step() {
      bool do_reset = true;
      typedef sequence_type::state state;

      for (iterator_type s = start(); s != this->sequences().end(); ++s) {
        // See class docs for why this is the least bad solution.
        sequence_type::step_state ret = NERVE_CHECK_PTR(*s)->sequence_step();

        if (ret == state::buffering) {
          // TODO:
          //   If there are multiple sequences doing buffering, then we ignore one
          //   of them here.
          start(s);
          do_reset = false;
        }
        else {
          NERVE_ASSERT(ret == state::complete, "there are no other possibilities");
        }
      }

      if (do_reset) {
        reset_start();
      }
    }

    private:
    sequences_type &sequences() { return sequences_; }

    typedef sequences_type::iterator iterator_type;

    bool buffering_sequence() { return start() == sequences().begin(); }

    void reset_start() { start_ = sequences().begin(); }
    void start(iterator_type i) { start_ = i; }
    iterator_type start() { return start_; }

    iterator_type start_;
    sequences_type &sequences_;
  };
}
#endif
