// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PROGRESSIVE_BUFFER_HPP_5bymbxa6
#define PIPELINE_PROGRESSIVE_BUFFER_HPP_5bymbxa6

#include "packet.hpp"
#include "packet_return.hpp"
#include "simple_stages.hpp"
#include "pipe_junction.hpp"

#include "../util/asserts.hpp"
#include "../util/indirect.hpp"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/type_traits/remove_pointer.hpp>

namespace pipeline {
  struct process_stage;

  /*!
   * \ingroup grp_pipeline
   *
   * Algorithm object for the following goals:
   *
   * - enforces the constant delay rule
   * - stages can produce any number of packets
   * - buffers don't expand (i.e if something is debuffering then no more input is
   *   added)
   *
   * The drawback is that there is overhead for stages which will only ever return
   * one or less packet.
   */
  class progressive_buffer {
    public:
    typedef packet_return stage_value_type;

    //! A matching destructor for allocation done in create_stage (which is
    //! necesary because the stages can be differing sizes).
    struct tracked_destructor {
      void operator()(process_stage *p) { pooled::tracked_byte_free(p); }
    };

    typedef pooled::container<process_stage*>::vector vector_type;
    typedef ::indirect_owned<vector_type, tracked_destructor> stages_type;

    typedef stages_type::value_type stage_type;
    typedef stages_type::iterator iterator_type;

    /*!
     * Note: the pipe_junction is used so that a) this object is not coupled to
     * the sequence object, and b) so that the pipe stuff and be protected.
     */
    progressive_buffer(pipe_junction &junc)
    : junc_(junc) {}

    /*!
     * Prepare the initial state.  This is a bit messy due to initialisation
     * outside of a constructor, but we can't really get around it because the
     * stage sequence isn't initialised by its constructor.  We can solve this
     * until the other has been solved.
     *
     * TODO:
     *   Might be necessary to mess with the connectors here.
     */
    void finalise() { this->reset_start(); }

    //! This resets the progressive buffer (not the stages) for When the stages
    //! themselves will be abandoned.  This means all the buffers will be empty, so
    //! we need to go to the start again.
    void abandon_reset() { this->reset_start(); }

    //! Is there a buffering stage here?
    bool buffering() { return this->start() != this->stages().begin(); }

    /*!
     * Perform an iteration of the stages where no stage is visited twice (but
     * some might be unvisited).  Pulls data from the input queue only when
     * necessary.
     */
    void step();

    //! Stored stages.  Stages are stored here so we can control the iterators.
    stages_type &stages() { return stages_; }

    private:

    //! Iterator state
    //@{
    iterator_type start() const { return start_; }
    void start(iterator_type i) { start_ = i; }
    void reset_start() { start(this->stages().begin()); };
    //@}

    //! Input/output
    //@{

    //! Also resets buffering stage next time if necessary.
    packet *debuffer_input();
    packet *read_input() { return junc_.read_input(); }
    void write_output(packet *p) { junc_.write_output(p); }

    //@}

    pipe_junction &junc_;
    stages_type stages_;
    iterator_type start_;
  };

} //ns pipeline


#endif
