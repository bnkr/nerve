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

    event_type event() const { return event_; }
    void event(event_type e) { event_ = e; }

    //! Is it a packet which causes a wipe?
    bool wipe() const { return event_ == event::abandon; }

    bool non_data() const { return event_ != event::data; }

    private:

    event_type event_;
  };
}

#endif
