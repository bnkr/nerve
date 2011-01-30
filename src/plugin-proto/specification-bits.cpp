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
    //   This is probably slower than simply adding this buffer, though.
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

    this->output(pkt, input_events_);
    // This is still a bit messy because we know for certain that the output
    // stage will pass on a single packet unmodified.  It would be ideal to
    // simply leave the stage-to-stage local pipes as they are and then call all
    // the observer stages, but this entails a checking overhead for every stage
    // so this way will work out faster.
    //
    // TODO:
    //   This could be fixed by relaxing the requirement on stages to always
    //   communicate to another thread.  This is entirely do-able beccause the
    //   input connector already has to be virtual because the input stage might
    //   need to read from a local pipe.  The output connector too because it
    //   might be the final terminator.  Furthermore, it doesn't actually change
    //   how the jobs are executed.
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
