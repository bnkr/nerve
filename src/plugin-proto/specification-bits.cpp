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


// ///////////////////////////// //
// Stage Sequence: Communication //
// ///////////////////////////// //

// like queues
class thread_pipe {
  void write(packet);
  packet read();
};

class local_pipe {
  void write(packet);
  packet read();
};

// ////////////////////////// //
// Stage Sequence: Connectors //
// ////////////////////////// //

// these fullfil the "terminator" requirements, and also some of the stage
// sequence requiremnts (regarding pipes etc).

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

// //////////////////////////////// //
// Stage Sequence: General Sequence //
// //////////////////////////////// //

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

  // When abandoning.  This means all the buffers will be empty, so we need to
  // go to the start again.
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


// Problems:
//
// - we have the "input stage is weird" problem again
//   - we already came up with a couple of ideas to solve that one, the most
//     sensible being multiple types of sequence
//   - this implementation actually relaxes the "connections must be thread
//     pipes" condition, so it would be even easier to entirely separate
//     differnt kinds of stages and thus solve the "stages matching" problem in
//     its entireity.
//
// This is a generalised sequence implementation which can handle any kind of
// stage.  It respects constant delay between stages within the same thread (i.e
// when the output pipe is local).  It is, of course, totally over the top for
// observer stages.
class stage_sequence {
  progressive_buffer data_loop;

  // Constant delay is guaranteed by the use of the progressive buffering loop.
  void run() {
    // This is necessary to discover non-data events as quickly as possible.
    // Otherwise a buffering stage will delay the input connector operation.
    packet = in_conn->read_non_data();
    if (packet) {
      non_data_loop();
    }
    else {
      data_loop.step();
    }
  }

  // Constant delay is always enforced because special events don't do anything.
  void non_data_loop(pkt) {
    switch (pkt.event) {
      case flush:
        data_loop.flush_reset();
        std::for_each(...);
        break;
      case abandon:
        data_loop.abandon_reset();
        std::for_each(...);
      case finish:
        ...;
      case load:
        data_loop.abandon_reset();
        // TODO:
        //   "Input stage is weird" problem.  Essentially it doesn't work at
        //   all unless we have a special "input stage sequence".
        //
        // a tricky one again... it's a shame we need all this ubercode in here
        // to deal with it, but in fairness said code never actually gets
        // executeed mostof the time, and the switch here should be a
        // branchtable...
        break;
    }

    // this is a new requirement on output connectors
    out_conn->wipe(pkt);
  }
};

// //// //
// Jobs //
// //// //

class job {
  stage_sequences_type sequences_;

  void job_thread() {
    std::for_each(sequences().begin(), sequences().end(), &sequence::synchronise);
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
// Process Stage //
// ///////////// //

class simple_stage {
  void data(pkt, outputter) = 0;
  // propogating events is fullfilled by the stage sequence
  void abandon() = 0;
  void flush() = 0;
  void finish() = 0;
};

class process_stage : simple_stage {
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

class thread_outputter : outputter{
  thread_pipe p_;
  void write(packet);
};

// this organisation means we don't need to deletgate -- there's only one
// virtual call for each of the stage methods and two for the data method.
class output_stage_base : simple_stage {
  void output(pkt, outputter) = 0;
  void reconfigure(pkt) = 0;

  // A pipe to the input thread.  This can be local or threaded.
  outputter input_events_;

  void data(pkt, outputter) {
    // fullfills the 'reqconfigure' event.  Somebody has to check it.  If we do
    // it here then it avoids type checking of all stages.

    if (pkt.configuration != last_configuration) {
      this->reconfigure(pkt);
    }

    // TODO:
    //   This method is mssy and a bit weird, but it's difficult to do while
    //   having only one kind of stage sequence.  Multiple stage sequences is
    //   extremely difficult to do becuase of constant delay rule.  Essentially
    //   we'd need another layer of buffering (i.e thread pipe emulation) in
    //   sequences which are connected and in the same thread.
    //
    //   We should re-name things:
    //
    //   - input stage sequence
    //     - in input and optional processors
    //   - process stage sequence
    //     - processors only
    //   - output stage sequence
    //     - an output and also observers
    //   - observer stage sequence
    //     - observers only
    //
    //   The spec has these concepts, but we don't have concrete classes for
    //   them and the spec does not a sequence to be exslusively one of these
    //   things.  We also have the "thread pipe" requirement on internally
    //   connect stage sequence outputs which we can't have if soem sequences
    //   are local.
    this->output(pkt, input_events_);
    outputter.write(pkt);
  }
};

class output_stage : output_stage_base {
};

// ////////////// //
// Observer Stage //
// ////////////// //

class observer_stage : simple_stage {};

// /// //
// Job //
// /// //

class job {
  void run() {
    std::for_each(stage_sequences.begin(), stage_sequences.end(), stage_seq_base::run);
  }
}
