// //////////// //
// the pipeline //
// //////////// //

// conceptual only
struct pipeline {
  stage_list stages_;
};

struct packet {
  event_id event_;
  // various other fields
};

// Informational data attached to a packet.
struct packet_return {
  packet_return() : buffering_(false), packet_(NULL) {}

  bool buffering() const { return buffering_; }
  bool empty() const { return packet_ != NULL; }
  packet *packet() { assert(! this->empty()); return packet_; }

  private:
    bool buffering_;
    packet *packet_;
};

// ///// //
// Pipes //
// ///// //

// like queues
class thread_pipe {
  void write(packet);
  bool would_block();
  packet *read_if_non_data();
  packet *read();
};

class local_pipe {
  void write(packet);
  packet read();
};

// ////////// //
// Connectors //
// ////////// //

// these fullfil the "terminator" requirements, and also some of the stage
// sequence requiremnts (regarding pipes etc).
//
// TODO:
//   These are either obsolete or wrong depending on how sections and sequences
//   are done because we can't guarantee connections have thread pipes if there
//   a multiple entities in one thread.

class in_connection_base {
  thread_pipe in_q_;
  void begin(local_queue q) = 0;
};

class in_terminator : in_connection_base {
  packet begin() { return pkt = q.read(); }
};

class in_connector : in_connection_base {
  packet begin() { return pkt = q.read(); }
};

class out_connection_base {
  virtual void end(pkt) = 0;
  virtual void flush() = 0;
};

// deletes packets
class out_terminator : out_connection_base {
  // so messy, and why do it?
  void end(pkt) {
    free(pkt);
  }

  void flush() {}
};

// propogates events and packets
class out_connector : out_connection_base {
  output_pipe p_;

  void end(pkt) {
    p_.write(pkt);
  }

  // flush has special requirements here
  void flush_end(pkt) { end(pkt); p_.clear(); }
};

// Junction between two connectors.
class junction {
  public:

  junction(in_connection &in, out_connection &out) : in_(in), out_(out) {}

  in_connection &in_;
  out_connection &out_;
};


// ////////////////////////////// //
// Generalised Progressive Buffer //
// ////////////////////////////// //

// Algorithm object for the following goals:
//
// - enforces the constant delay rule
// - stages can produce any number of packets
// - buffers don't expand (i.e if something is debuffering then no more input is
//   added)
//
// The drawback is that there is overhead for stages which will only ever return
// one or less packet.
class progressive_buffer {
  public:

  typedef std::vector<process_stage*> stages_type;
  typedef stages_type::iterator iterator_type;
  typedef boost::remove_pointer<stages_type::value_type> stage_type;

  // Stages must be fully initialised.
  progressive_buffer(stages_type &stages, junction &pipes)
  : stages_(stages), pipes_(pipes)
  {
    NERVE_ASSERT(! stages.empty(), "stages must be ready (read iterable) already");
    start_i = this->stages().begin();
  }

  // This resets the progressive buffer (not the stages) for When the stages
  // themselves will be abandoned.  This means all the buffers will be empty, so
  // we need to go to the start again.
  void abandon_reset() {
    start_ = stages().begin();
    std::for_each(stages().begin(), stages().end(), boost::bind(&stage_type::clear, _1));
  }

  // Is there a buffering stage here?
  bool buffering_stage() { return start_i != this->stages().begin(); }

  // Perform an iteration of the stages where no stage is visited twice (but
  // some might be unvisited).  Pulls data from the input queue only when
  // necessary.
  void step() {
    packet *input;
    iterator_type real_start = start_;

    if (this->buffering_stage()) {
      ++real_start_;
      input = this->debuffer_input();
    }
    else {
      input = this->read_input();
    }

    const iterator_type end = stages().end();

    for (s = real_start; s != end; ++s) {
      NERVE_ASSERT(! input->is_non_data(), "can't be doing a data loop on non-data");
      const stage_return ret = s->process(input);

      NERVE_ASSERT(! (ret.empty() && ret.buffering()), "empty xor buffering");
      if (ret.empty()) {
        return;
      }
      else if (ret.buffering()) {
        // Simply assigning this every time we meet a buffering stage means we
        // end up debuffering the latest stage which has stuff to debuffer.
        start_ = s;
      }

      input = ret.packet();
    }

    this->pipe_output(input);
  }

  private:

  packet *pipe_input() { return pipes_.in().read(); }
  void pipe_input(packet *p) { return pipes_.out().write(p); }

  //! Also resets buffering stage next time if necessary.
  void debuffer_input() {
    const stage_return ret = input = start_i->debuffer();

    NERVE_ASSERT(! ret.empty(), "empty data from a buffering stage is forbidden");

    if (! ret.buffering()) {
      start_ = stages().begin();
    }

    return ret.packet();
  }

  stages_type &stages() { return stages_; }
  jumnction &pipes() { return pipes_; }

  stages_type &stages_;
  junction pipes_;
  iterator_type start_;
};

// ////////////// //
// Stage Sequence //
// ////////////// //

class stage_sequence {
  void sequence_step() = 0;

  // Abstract the nature of output.
  in_connector_base *input_;
  out_connector_base *output_;

  protected:

  template<class Container, class MenFn>
  void call_member(const Container &c, MemFn memfn) {
    std::for_each(c.begin(), c.end(), boost::bind(memfn, _1));
  }

  // Calls relevant methods for a non-data event and then propogates the packet.
  // Constant delay is always enforced because special events don't do anything.
  template<class Container>
  void non_data_step(const Container &c, packet *pkt) {
    typedef Container::value_type value_type;
    switch (packet.event()) {
      case packet::ev_flush:
        this->call_member(c, boost::mem_fn(&value_type::flush));
        break;
      case packet::ev_adandon:
        this->call_member(c, boost::mem_fn(&value_type::abandon));
        break;
      case packet::ev_finish:
        this->call_member(c, boost::mem_fn(&value_type::finish));
        break;
      default:
        NERVE_ABORT("not a non-data event");
        break;
    }
    output_->wipe(pkt);
  }
};

// Specialist stage sequence for dealing with input event stages.  This is
// necesasry to avoid having every stage check if their packets are and because
// the input stage results in the creation of non-data events.
class input_stage_sequence  : stage_sequence {
  void sequence_step() {
    packet = in_term->begin();
    if (packet.type == load) {
      // buffered version:
      is_.load(details);
      pkt.event = flush;
    }
    else if (packet.type == skip) {
      is_.skip();
      packet.type = flush;
    }
    else if (...) {
      ...
    }

    out_con->write(packet);
  }
};

// Simple sequence which can only handle those stages that look but don't touch.
class observer_stage_sequence : stage_sequence {
  void sequence_step() {
    packet = in_conn->read();

    if (packet.event() == packet::e_data) {
      std::for_each(
          stages().begin(), stages().end(),
          boost::bind(&observer_stage::observe, _1, packet)
      );
    }
    else {
      this->non_data_step(stages(), packet);
    }

    out_conn->write(packet);
  }
};

// The most generals sequence.  It can handle any number of stages, all of which
// can do some buffering or withhold packets.
class process_stage_sequence : stage_seqeuence {
  progressive_buffer data_loop;

  // Constant delay is guaranteed by the use of the progressive buffering loop.
  void sequence_step() {
    // Checking for non-data only is necessary to discover non-data events as
    // quickly as possible.  Otherwise a buffering stage will delay the input
    // connector operation.
    //
    // TODO:
    //   This could be omtimised if we could iterate all stages in one go.  Then
    //   the section could check for most non-data events.  That would mean no
    //   need to pass flushes etc. down the local pipes so less checks all
    //   round.  There are slight problems with the input stage though (because
    //   it can "create" nd-events).
    //
    // TODO:
    //   It would improve latency further if we did this at the end of each
    //   stage.  Of course, check is useless on a local pipe...
    packet = in_conn->read_if_non_data();
    if (packet) {

      // TODO:
      //   Sub-optimal.
      if (packet.type == abandon) {
        data_loop.abandon_reset();
      }
      this->non_data_step(packet);
    }
    else {
      data_loop.step();
    }

    return data_loop.debuffering() ? status_debuffer : status_continue;
  }
};

// /////// //
// Section //
// /////// //

// The section class contains multiple types of stages via the mono-typed
// sequence container and always communicates with other threads (excepting the
// input stage which might not).  The section is necessary:
//
// - otherwise a job can't tell where thread communication boundaries are and
//   therefore can't ensure one block per iteration (which is necesasry to stop
//   deadlocks)
// - allow sequences to be specialised to each type of stage (which is necessary
//   to avoid unnecessary work and to deal with special stages without massive
//   hax)
// - we can't have sequences defining thread boundaries (which would solve the
//   first requirement) because of the second requirement
//
// This implementation of a section totally leaves any communication of packets
// to the sequences themselves (they'll use a "connector" abstraction).  This
// means we need no special handling for buffering stages or passing of data
// which is good because different stage types have different requirements on
// this.  If we did any of that stuff here you'd end up with the same issue of
// type requirements not matching exactly (e.g pointless progressive buffering
// of observer stages).
class section {
  sections(container &sequences) {
    // relies that we have the data first of course or the iterator is invalid!
    start_i = sequences().begin();
  }

  bool would_block() {
    return
      start_i == stages().begin() &&
      // It's only necessary to check the input because we can assume that
      // something is waiting for the output (or will be eventually) and won't
      // be in the same thread.
      start_i->in_connector().would_block();
  }

  void section_step() {
    for (s = start_i; s != this->sequences().end(); ++s) {
      if (s->sequence_step() == buffering) {
        start_i = s;
        return;
      }
    }

    start_i = this->sequences().begin();
  }
};

// //// //
// Jobs //
// //// //

// Maps onto a thread, contains sections, and ensures we don't block too many
// times in a thread (which causes deadlocks).
class job {
  sections_type sections_;

  void job_thread() {
    for (;;) {
      bool blocked_already = false;
      for (s = sections().begin(); s != sections().end(); ++i) {
        // Fullfills the "jobs mustn't block twice" requirement.
        //
        // TODO:
        //   The mechanism and purpose is not well defined.  Especially we might
        //   care if blocking on input or output.  It might be that we do it
        //   based on the return from section_step and let the messy details of
        //   that be totally transparent.
        //
        if (s->would_block()) {
          if (blocked_already) {
            // Continue instead of break to avoid a bias towards sections at the
            // start of the job.
            continue;
          }
          else {
            blocked_already = true;
          }
        }

        i->section_step();
      }
    }
  }
};

// /////////// //
// Input Stage //
// /////////// //

// requirements about reading are fullfilled by the initial_stage_sequence so we
// don't need polymorphic bits.
class input_stage {
  // note: we can do a complicated API because we know that a stage sequence will
  // always have the input at its start -- it means a virtual call though.
  void pause();
  void skip(where);
  void load(file);
  void read(local_q);
  void finish();
  // possibly others
};

// ///////////// //
// Simple Stages //
// ///////////// //

class simple_stage {
  // propogating events is fullfilled by the stage sequence
  void abandon() = 0;
  void flush() = 0;
  void finish() = 0;
};

class process_stage : simple_stage {
  packet_return process(packet *) = 0;
  packet_return bebffer() = 0;
};

class observer_stage : simple_stage {
  void observe(packet*);
};

// //////////// //
// Output Stage //
// //////////// //

class outputter {
  virtual void write(packet) = 0;
};

class local_outputter  : outputter {
  local_pipe p_;
  void write(packet);
};

class thread_outputter : outputter {
  thread_pipe p_;
  void write(packet);
};

// this organisation means we don't need to deletgate -- there's only one
// virtual call for each of the stage methods and two for the data method.
class output_stage : observer_stage {
  void output(pkt, outputter) = 0;
  void reconfigure(pkt) = 0;

  // A pipe to the input thread.  This can be local or threaded.
  outputter input_events_;

  void observe(packet *pkt) {
    // fullfills the 'reconfigure' event.  Somebody has to check it.  If we do
    // it here then it avoids type checking of all stages.  Additionally, it
    // means we can later have a reconfgiure event which a configuration chaning
    // stage could deal with.

    // TODO:
    //   Perhaps better in a hypothetical output seqeuence.

    if (pkt.configuration != last_configuration) {
      this->reconfigure(pkt);
    }

    // hmmm...
    this->real_observe(pkt);
  }
};

/****** old bits *****/

// ////////////////////////////////////////////////////////// //
// Stage Sequence: Sequence: Two Sequences and one Terminator //
// ////////////////////////////////////////////////////////// //

// This polymorphic solution is used to solve the awkwardness of the input
// stage, the main
//
// - it is not a simple stage (in that is expects events)
// - it implies the production of other events -- its own events are translated
//   into events for the simple stages
// - it can output multiple packets
//
// One solution would have been to put the input stage in the terminator, which
// would remove the need for this polymorphic stuff.  Here's why it won't work:
//
// - every in connector needs to use a local queue, but only the input stage
//   could ever produce multiple packets
// - calls to begin() are always virtual
// - translated event packet caused by the skip events etc. will be tested again
//   in the stage sequence though we know what operation we have do do because we
//   just set the event type.
// - in connectors need to be polymorphic anyway (so either way you get a
//   virtual call)
class basic_stage_sequence {
  void run() = 0;

  local_queue spare_buffer;

  void single_data_event(packet) {
    // TODO:
    //   This buffering is annoying and unnecessary, but it's hard to see how we
    //   might integrate this into the existing algorithm.  A solution might be
    //   to have the entire multi-data implementation in a loop, i.e
    //
    //     q.each {|e| progressive_process(e); }
    //
    //   That is probably slower than simply adding this buffer, though.
    spare_buffer.push(packet);
    processive_process_data(spare_buffer);
  }

  // We must buffer somewhere, and it's less complicated to do it here.  In
  // fact, if it's in the stage then it's very easy to never actually see that
  // buffered data because the start connector will block until it has more
  // packets.  The 'big buffering' stage would have to keep storing the new data
  // and outputting the old.  There's also problems with flushing.  Let's just
  // not go there ^^.
  struct stage_and_buffer {
    local_queue output;
    simple_stage *stage;
  };
  std::vector<stage_and_buffer> stages_;

  // This arrangement passes a single packet along the sequence instead of
  // having a full buffer step each time.
  //
  // This means that each stage can output as many packets as it likes but we
  // only do one input per stage-iteration.  This is crucial because every
  // "while buffering" loop will put something out on the output connection
  // (unless all data is consumed by the stages).  This means that the pipeline
  // won't starve if, for some reason, a stage outputs masses of packets.
  //
  // The loop also works progressively, so we stop considering stages whoose
  // output has been processed fully.
  //
  // In nerve's terminology, this implementation enforced the "constant delay
  // rule".
  void progressive_process_data(local_pipe *initial_in) {
    iter_type beg = stages().begin();
    iter_type end = stages().begin();

    bool buffering;
    local_queue *in = initial_in;
    do {
      packet *pkt = NULL;
      buffering = false;
      iter_type s = beg;
      while (s != end) {
        pkt = take_top(*in);
        s->stage->data(pkt, s->buffer);
        const size_t outputted = s->buffer.size();

        // no more processing possible
        if (outputted == 0) goto finish;

        ++s;

        // set a bookmark where we know there is buffered data to start at next time
        if (! buffering && outputted > 1) {
          buffering = true;
          beg = s;
        }

        in = &s->buffer;
      }

      // This is a necessary buffered step because a) that s->buffer might have
      // multiple elements in it and b) the only way to output without buffering
      // is for the stage to do the connector's work.  This would mean a
      // polymorphic interface to do the outputting.  On balance, the method
      // without polymorphism should be faster because most of the time it's
      // just a couple of bound checks on the queue.
      end_connector->end_data(take_top(in));
    } while (buffering);

finish: ;
  }

  void flush_event() {
    std::for_each(simple_stages.begin(), simple_stages.end(), mem_fcn(&simpe_stage::flush));
    end_term->end_flush();
  }


  void take_top(local_pipe q) {
    packet p = q.top();
    q.pop();
    return p;
  }
};

// sequence which contains the input stage
class initial_stage_sequence : basic_stage_sequence {
  input_stage is_;

  void run() {
    packet = in_term->begin();
    if (packet.type == load) {
      // buffered version:
      is_.load(details);
      is_.read(buffer[0]);
      multi_data_event(buffer[0], buffer[1]);

      // progressive version:
      local_q out;
      is_.read(out);
      progressive_process_data(out);
    }
    else if (packet.type == skip) {
      is_.skip();
      flush_event();
    }
    else if (...) {
      x_event();
    }
  }

};

// TODO:
//   See todos in the output stage regarding having several types of stage
//   sequence which match up more cloesly with the stages and variety of
//   operations therein.

// sequence which is not the initial stage
class connecting_stage_sequence : basic_stage_sequence {
  void run() {
    pkt = in_term->begin(in);

    if (pkt.event == data) {
      single_data_event(pkt);
    }
    else if (event == flush) {
      flush_event();
    }
    else if (...) {
      x_event();
    }
  }
};
