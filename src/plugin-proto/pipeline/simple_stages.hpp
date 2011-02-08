// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * All the basic polymorphic stage types.
 */

#ifndef PIPELINE_STAGES_HPP_jpqdrv7d
#define PIPELINE_STAGES_HPP_jpqdrv7d

namespace pipeline {
  struct packet;
  struct packet_return;

  /*!
   * \ingroup grp_pipeline
   * The basic event-operations supported by all data stages.
   */
  class simple_stage {
    public:
    // propogating events is fullfilled by the stage sequence
    virtual void abandon() = 0;
    virtual void flush() = 0;
    virtual void finish() = 0;
  };

  /*!
   * \ingroup grp_pipeline
   * A stage which can do anything with the packet.
   */
  class process_stage : public simple_stage {
    public:
    virtual packet_return process(packet *) = 0;
    virtual packet_return debuffer() = 0;
  };

  /*!
   * \ingroup grp_pipeline
   * A stage which merely watches the packet.
   */
  class observer_stage : public simple_stage {
    public:
    virtual void observe(packet*) = 0;
  };
}

#endif
