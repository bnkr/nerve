// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb
#define PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb

#include "simple_stages.hpp"
#include "packet.hpp"
#include "pipe_junction.hpp"

#include "../util/asserts.hpp"

#include <algorithm>
#include <boost/bind.hpp>

namespace config { class stage_config; }

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

    //! Note: see docs for class section to explain why outputted packets is
    //! done with the connector abstraction.
    virtual step_state sequence_step() = 0;

    /*!
     * Initialise and store the proper stage for the given config.  Pointer
     * remains valid.  This is done as a virtual function so that allocation and
     * type-safety can be specialised to the sub-class.
     */
    virtual simple_stage *create_stage(::config::stage_config &) = 0;

    //! Prepare to run; set up initial state etc.
    virtual void finalise() = 0;

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

    //! The input/output pipe for this sequence.
    pipe_junction &junction() { return junction_; }

    packet *read_input() { return junction_.read_input(); }
    void write_output(packet *p) { junction_.write_output(p); }
    void write_output_wipe(packet *p) { junction_.write_output_wipe(p); }

    //@}

    private:

    // TODO:
    //   Not 100% sure this will be here by value.  Depends on how the
    //   create_sequence assigns.  It is definitely certain that we need some
    //   special considerations because of the way the object is initialised
    //   gradually.
    pipe_junction junction_;
  };
}

#endif
