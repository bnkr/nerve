// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb
#define PIPELINE_STAGE_SEQUENCE_HPP_7dmcjdrb

#include <algorithm>
#include <boost/bind.hpp>

#include "simple_stages.hpp"
#include "packet.hpp"
#include "../asserts.hpp"

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

    // TODO:
    //   This is probably ok but let's leave it until the connectors are totally
    //   done.
    packet *read_input();
    // TODO:
    //   Connectors are still a bit unsure in this situation.  It might be that
    //   we can return data from sequence_step.  I think not though, because
    //   that entails havign a +debuffer+ method on the sequence as well or
    //   having various checks to see if data is actually returned or not.  All
    //   depends on how connectors are done, so let's sort it out later.
    void write_output(packet *);
    // TODO:
    //   Should replace the entire output queue with the packet.  This is unsure
    //   now because we don't know exactly how connectors are going to work.
    void write_output_wipe(packet*);

    private:

  };
}

#endif
