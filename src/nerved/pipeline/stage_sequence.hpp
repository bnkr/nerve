// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb
#define PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb

#include "simple_stages.hpp"
#include "packet.hpp"
#include "connection.hpp"
#include "local_pipe.hpp"

#include "../util/asserts.hpp"

#include <algorithm>
#include <boost/bind.hpp>

namespace stages { class stage_data; }

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * The base class and some utility methods for a stage sequence.  Specialised
   * classes make minimal sequences for use with a particular type of stage.
   */
  class stage_sequence {
    public:

    //! A namespace for the sequence state enum.
    struct state {
      enum step_state {
        buffering,
        complete
      };
    };
    typedef state::step_state step_state;
    typedef stages::stage_data stage_data_type;
    typedef polymorphic_connection connection_type;

    stage_sequence() : pipe_used_(false) {}

    //! Note: see docs for class section to explain why outputted packets is
    //! done with the pipe abstraction.
    virtual step_state sequence_step() = 0;

    /*!
     * Initialise and store the proper stage for the given config.  Pointer
     * remains valid.  This is done as a virtual function so that allocation and
     * type-safety can be specialised to the sub-class.
     */
    virtual simple_stage *create_stage(stage_data_type &) = 0;

    //! Prepare to run; set up initial state etc.
    virtual void finalise() = 0;

    //! This may or may not get used, but I happen to know that the object is
    //! trivial so we may as well store it here.
    local_pipe *create_local_pipe() {
      NERVE_ASSERT(! pipe_used_, "a sequence can only release a single local pipe");
      pipe_used_ = true;
      return &local_pipe_;
    }

    //! The input/output pipe for this sequence.
    connection_type &connection() { return connection_; }

    protected:

    //! Call a method on every member of the container.
    template<class Container, class MemFn>
    void call_member(const Container &c, MemFn memfn) {
      std::for_each(c.begin(), c.end(), boost::bind(memfn, _1));
    }

    // Calls relevant methods for a non-data event and then propogates the packet.
    // Constant delay is always enforced because special events don't do anything.
    template<class Container>
    void non_data_step(const Container &c, packet *pkt) {
      typedef typename Container::value_type value_type;
      switch (NERVE_CHECK_PTR(pkt)->event()) {
      case packet::event::flush:
        // Can't use stage_type::flush for some reason.
        this->call_member(c, &simple_stage::flush);
        break;
      case packet::event::abandon:
        this->call_member(c, &simple_stage::abandon);
        break;
      case packet::event::finish:
        this->call_member(c, &simple_stage::finish);
        break;
      default:
        NERVE_ABORT("not a non-data event");
        break;
      }

      write_output_wipe(pkt);
    }

    //! Input/output convenience
    //@{

    packet *read_input() { return connection_.read_input(); }
    void write_output(packet *p) { connection_.write_output(p); }
    void write_output_wipe(packet *p) { connection_.write_output_wipe(p); }

    //@}

    private:
    connection_type connection_;
    bool pipe_used_;
    local_pipe local_pipe_;
  };
}

#endif
