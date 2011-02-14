// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PACKET_HPP_7nyok8p6
#define PIPELINE_PACKET_HPP_7nyok8p6

namespace pipeline {

  //! \ingroup grp_pipeline
  //! Data packet passed down the pipeline.
  class packet {
    public:

    //! A namespace for the event identifiers.
    struct event {
      enum id {
        data,
        load,
        skip,
        flush,
        abandon,
        finish
      };
    };

    typedef event::id event_type;

    event_type event() const;
    bool non_data() const;
  };
}

#endif
