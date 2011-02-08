// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_SECTION_HPP_1auycg4v
#define PIPELINE_SECTION_HPP_1auycg4v

#include "stage_sequence.hpp"

#include <vector>
#include <boost/type_traits/remove_pointer.hpp>

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * The section class contains multiple types of stages via the mono-typed
   * sequence container and always communicates with other threads (excepting the
   * input stage which might not).
   *
   * Why the The section is necessary:
   *
   * - otherwise a job can't tell where thread communication boundaries are and
   *   therefore can't ensure one block per iteration (which is necesasry to stop
   *   deadlocks)
   * - allow sequences to be specialised to each type of stage (which is necessary
   *   to avoid unnecessary work and to deal with special stages without massive
   *   hax)
   * - we can't have sequences defining thread boundaries (which would solve the
   *   first requirement) because of the second requirement
   *
   * This implementation of a section totally leaves any communication of packets
   * to the sequences themselves (they'll use a "connector" abstraction).  This
   * means we need no special handling for buffering stages or passing of data
   * which is good because different stage types have different requirements on
   * this.
   *
   * The result is that for every sequence there is a virtual call to write
   * output, but but we don't need to do a buffering/no-return check, which is
   * particularly annoying for those sequences which never buffer and/or always
   * return.  You'd also have to manage passing the packet on to the next
   * sequence, which begins to duplicate the progressive buffer stuff wich the
   * sequence already does and probably means we need a special +debuffer+
   * method on sequences which is further duplication.  There's also the issue
   * that we have to check the event type to decide whether to clear the output
   * queue.
   *
   * It's a bit hard to see if the current method is faster, but it does seem
   * tobe a lot neater.
   *
   * TODO:
   *   It might turn out that the various extra checks done here are quicker,
   *   but it's something we really need to measure.
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
