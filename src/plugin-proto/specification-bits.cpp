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

// ////////////////////////////// //
// Generalised Progressive Buffer //
// ////////////////////////////// //

// Algorithm object which enforces the constant delay rule when iterating over
// stages which may produce any numberof output packets.  The drawback to this
// method is that the buffers gradually increase in size until we run out of
// input or the data is explicitly abandoned.  This happens in any case where
// multiple outputs are allowed, though, and this implementation means that
// stages don't have to care about doing their own buffering
class progressive_buffer {
  iterator more_output_i;

  struct buffering_wrapper {
    local_pipe q;
    state *s;
  };
  vector<buffering_wrapper<stage> > stages;

  // Perform an iteration of the stages where no stage is visited twice (but
  // some might be unvisited).  Pulls data from the input queue only when
  // necessary.
  void step() {
    start_stage, start_data = initialise_loop();
    iterate_once(start_stage, start_data);
  }

  // Call when the stages are going to flush their data.  This doesn't need to
  // do anything yet.
  void flush_reset() { }

  // This resets the progressive buffer (not the stages) for When the stages
  // themselves will be abandoned.  This means all the buffers will be empty, so
  // we need to go to the start again.
  void abandon_reset() {
    more_input_i = stages().end();
    stages.each {|buffer| buffer.clear }
  }

  private:

  // Find the first stage to run and the data to start with.  This might come
  // from buffered data, or from the input connection.
  void initialie_loop() {
    packet *start_data;
    iterator start_stage;

    // Constant delay part 1: don't introduce more data into the pipeline until
    // the buffers are clean.
    if (more_output_i == stages.end()) {
      start_data = in_conn.read;
      start_stage = stages.begin();
      more_output_i = start_stage;
    }
    else {
      start_data = take_top(more_output_i->buffer);
      // Start processing with the stage after the one that still has buffered
      // data.
      start_stage = more_output_i++;
    }

    [start_stage, start_data]
  }

  // Visit every stage after some known point and produce some output.  Any
  // stage which outputs multiple packets will have those packets buffered.  The
  // constant delay rule is enforced by the buffering on the condition that new
  // packets are not inserted into the chain until all the old ones are gone.
  // There is always one output produced on the output connection per packet
  // input, providing no packet is consumed (but it is OK for packets to be
  // consumed).
  //
  // This is a highly general algorithm.  Stages can do anything with their
  // packets here and still have the constant delay rule.
  void iterate_once(start_stage, packet *start_data) {
    packet *input = start_data;
    for (s = start_stage; s != end; ++s) {

      // Constant delay part 2: only handle a single packet of returned data per
      // virit to a stage.
      //
      // TODO:
      //   Possible change: only read one packet.  Evenrything else is identical
      //   except the buffered data is handled by the stage (i.e it might not
      //   have any).
      s.stage->data(input, s.buffer);

      if (s.buffer().empty()) {
        goto finish;
      }

      input = take_top(ret);

      // This incurs overhead if the stage can only output a single packet, but
      // it does mean we can mix both generators and observers and that
      // generators which produce strictly one packet don't need a buffer at
      // all.
      //
      // TODO:
      //   This uses the earliest buffering stage.  We want to go from the
      //   latest buffering stage and then back to the one before it before
      //   introducing a new packet.  Currently this is the easier way.
      if (! bookmark_set) {
        if (! s.buffer.empty()) {
          // mark that this stage has more data to deal with
          more_output_i = stage;
          // make sure the bookmark isn't messed with by later stages
          bookmark_set = true;
        }
        else {
          // this stage does not need to be visited next step
          ++more_output_i;
        }
      }
    }


    out_conn.write(input);

finish:;
  }
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
    //   It would improve latency further if we did this at the end of each
    //   stage...
    //
    // TODO:
    //   Should this be done by the secton?  We don't know the connectors  are
    //   multi-threded because they might be terminators.  We know that the
    //   stage sequence is always mono-threaded, except that it might be the
    //   last stage sequence in a section which means that it is effectively
    //   multi-threaded.
    packet = in_conn->read_if_non_data();
    if (packet) {
      this->non_data_step(packet);
    }
    else {
      data_loop.step();
    }
  }
};

// /////// //
// Section //
// /////// //

// Definite stuff:
//
// * there must be progressive buffering in the section in order to remember
//   which is teh buffering sequence (i.e any sequence which has a buffering
//   stage)
//
// Conditional stuff:
//
// * if the section passes the input packet then there must be a separate
//   debuffer call because otherwise we can't avoid passing more inputs.
// * if the sequence returns a packet, we must also be able to tell whether the
//   sequence needs to be debuffered
//   - this is already a requirement of the sequence so it basically means we
//     need the same return data
//
// Observations:
//
// * we could always make the stage sequence totally dumb, so that all of the
//   progressive buffering is done here.  Would be nice to generalise the
//   progbuf algorithms to support moving it about wherever necessary.
// * the only remaining issue we have is how data is passed to and from the
//   sequence

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
class section {
  sequences_type sequences_;

  bool will_block();

  // Most basic.  Won't work because we don't know when to start at a particular
  // sequence.
  void section_step1() {
    for (s = sequences.begin(); s != sequences().end(); ++i) {
      s->sequence_step();
    }
  }

  iterator buffering_sequence;

  // Progressively buffered section iteration.  I wasn't expecting to need
  // progressive buffering here.  Perhaps the details of passing a packet to the
  // next stage could be handled only by the connectors?
  void section_step2() {
    if (buffering_seqeuence_i == sequences().end()) {
      buffering_sequence_i = sequences().begin();
      packet *in = in_pipe.read_non_data();
    }
    else {
      packet *in;
    }

    bool buffering_this_step = false;

    for (s = sequences.begin(); s != sequences().end(); ++i) {
      packet_return ret = s->sequence_step();

      if (ret.buffering()) {
        buffering_sequence = s;
      }
      else if (! buffering_this_step) {
        ++buffering_sequence;
      }

      // go to the next section
      if (ret.packet() == NULL) {
        return;
      }
    }

    out_pipe.write(pkt);
  }

  // Assume that all communication is handled by the connections.  This pretty
  // much equivilent to going it all tin the job class.  All the extra work
  // would be is a would_block check on each sequence.
  void section_step3() {
    for (s = start_i; s != sequences().end()) {
      sequence_type::status_type r = s->sequence_step();

      switch (r) {
        case sequence_type::status_buffering:
          start_i = s;
          goto next_section;
        case sequence_type::status_finished:
          break;
      }
    }

    // skipped if a sequence is buffering
    start_i = sequences().begin();

next_section: return;
  }

  // Handle connectors ourself and sequences have to have their own buffering
  // state.
  //
  // Both of the post sequence_step checks would have been done in the sequence
  // step.  Also, buffering and empty will never happen in observer stages.
  // Neither is any of the checks at the start of the function remotely useful
  // to ibservers.  Observers was pretty much the whole point of sections.
  void section_step4() {
    packet *input = NULL;
    if (start_i == sequences().end()) {
      input = in_conn->read();
      start_i = sequences().begin();
      s = sequences().begin();
    }
    else {
      input = start_i->debuffer();
      s = start_i++;
    }

    packet_return ret;
    for (; s != sequences().end()) {
      ret = s->sequence_step(input);

      NERVE_ASSERT(! (ret.buffering() && ret.empty()), "sequence state return be buffering xor empty");

      if (ret.buffering()) {
        start_i = s;
        goto next_section;
      }

      if (ret.empty()) {
        goto next_section;
      }
    }

    out_conn->write(ret.packet());

next_section: return;
  }

  // for section_step4
  bool would_block4() {
    return start_i == stages().begin() && start_i->connector().would_block();
  }
};

// section-step4
class process_stage_sequence {
  void sequence_step(packet *input) {
    stages().each
  }

  packet *debuffer() {
  }
};

// //// //
// Jobs //
// //// //

class job {
  sections_type sections_;

  void job_thread() {
    for (;;) {
      bool blocked_already = false;
      for (s = sections().begin(); s != sections().end(); ++i) {
        // Fullfills the "jobs mustn't block twice requirement.
        if (s->would_block()) {
          if (blocked_already) {
            // TODO:
            //   should we do a continue?  Other sections won't get visited
            //   which means we have a bias for the beginning sections.
            break;
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
