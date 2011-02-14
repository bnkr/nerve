// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_INPUT_STAGE_HPP_tfebw7wn
#define PIPELINE_INPUT_STAGE_HPP_tfebw7wn

namespace pipeline {
  struct packet;

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
  class input_stage {
    void pause();
    void skip(void *where);
    void load(void *file);
    packet *read();
    void finish();
    // possibly others
  };
}

#endif
