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
    virtual ~simple_stage() {}
    virtual void abandon() = 0;
    virtual void flush() = 0;
    virtual void finish() = 0;
    //! Recieve a key and a value configuration which was read from the config
    //! file.
    virtual void configure(const char *, const char *) = 0;
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

  /*!
   * \ingroup grp_pipeline
   *
   * Requirements about reading are fullfilled by the initial_stage_sequence so
   * we don't need polymorphic bits.
   *
   * We can do a specialised API because we know that at the very least a stage
   * sequence will always have the input at its start (usually there will be a
   * specialised stage sequence to deal with input).
   */
  class input_stage : public simple_stage {
    public:
    typedef void* skip_type;
    typedef const char * load_type;

    virtual void pause() = 0;
    virtual void skip(skip_type location) = 0;
    virtual void load(load_type where) = 0;
    virtual packet *read() = 0;
    virtual void finish() = 0;
  };
}
#endif
