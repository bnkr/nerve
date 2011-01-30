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

template<class Pipe>
class junction {
  Pipe in_;
  Pipe out_;
};

// like queues
class thread_pipe {
  void write(packet);
  packet read();
};

class local_pipe {
  void write(packet);
  packet read();
};

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

// ////////////////////////// //
// Stage Sequence: Connectors //
// ////////////////////////// //

// these fullfil the "terminator" requirements, and also some of the stage
// sequence requiremnts (regarding pipes etc).

class begin_connector {
  thread_pipe in_q_;
  packaet begin() = 0;
};

class initial_begin_connector {
  input_stage is_;

  begin() {
    q.read();
    if (q.event == read) {
      is_.read();
    }
  }
};

class continue_begin_connector {
  begin() { return q.read(); }
};

class end_connector {
  end(pkt) = 0;
};

// deletes packets
class final_end_connector {
  // so messy, and why do it?
  void end(pkt) {
    free(pkt);
  }
};

// propogates events and packets
class continue_end_connector {
  output_pipe p_;

  void end(pkt) {
    p_.write(pkt);
  }
};

// //////////////////////// //
// Stage Sequence: Sequence //
// //////////////////////// //


// we still probably need a "mono_stage_sequence" which handles the large
// differences between a sequence with only one stage.
class stage_sequence {

  // - removes the outputter
  // - removes the junction entirely
  // - does an unnecessary local_queue buffer step before calling end
  //   - the majority of the time that will equate to a single assignment and
  //     two rounds of bounds checking )one for insert and one for iterating)
  //   - it also removes the virtual outputter call
  // - stage_sequences are truely generic
  // - only two virtual calls per run
  //
  // I think I'll stick with this design for now.
  void run1() {
    pkt = begin_term->begin();
    if (pkt.event == data) {
      in, out = local_queue
      in << pkt
      each_stage {|s|
        in.each {|pkt|
          s.data(pkt, out);
        }
        swap(in, out)
      }

      in.each {|pkt|
        end_term->end(pkt);
      }
    }
    else if (event == finish) {
      stages.each {|s| s.finish }
      end_term->end(pkt);
    }
    // other events
  }
};

/********* older bits ********/

class basic_stage_sequence {
  junction<thread_pipe, thread_pipe> pipe_;
  // between each stage
  local_pipe buffers_[2];

  local_pipe *in, *out;

  stage_list stages_;

  // doesn't handle input stages
  void process_output_observe(pkt) {
    event = pkt.event
    if (event == data) {
      // TODO:
      //   Would this be faster as a virtual functtion for "mono_stage_sequence"
      //   or "multi_stage_sequence"?
      if (stages.size() > 1) {
        // this outputter stuff is wasted effort on observer stages, but it's
        // more trivial work compareed to doing if stagements on every stage.
        local_outputter o(*in);
        stage_[0].data_stage(packet, o);

        for (i = 1; i < stages.size() - 1; ++i) {
          in.each {|p|
            stage_.stage_data(p, outputter);
          }
          swap(in, out);
        }

        thread_outputter final_outputter(pipe_.out());
        in.each {|p|
          stage_.stage_data(p, outputter);
          (stages.end() - 1)->stage_data(p, final_outputter);
        }
      }
      else {
        thread_outputter final_outputter(pipe_.out());
        (stages.end() - 1)->stage_data(packet, final_outputter);
      }
    }
    else if (event == finish) {
      // no outputter necessary because we only ever propogate the packet
      each_stage.send(:finish);
      // here fullfilling requirements of a stage.
      propogate(pkt);
    }

    // fullfills end-terminator requirements
    //
    // TODO:
    //   This is horribly messy.  It needs to be polymorhic really.
    if (final_stage) {
      free(packet);
    }
  }
};

class stage_sequence : basic_stage_sequence {
  void run() {
    packet = pipe_.input();
    process_output_observe(pkt);
  }
};

// fullfils requirements of the begin terminator
class initial_stage_sequence {
  void run() {
    pkt = pipe.read;

    if (event == load) {
      // other input-ish events would work like this too.
      assert(stage[0].type == input);
      // here fullfilling requirements of an input stage.
      stage[0].load;
      pkt.event = flush;

      // do a normal run with a modified packet which flushes the sequence.
      // Actually this would be more efficiant written in this class
      parent::process_output_observe(pkt);
    }
    else if (event == (other input events)) {
      similar to above
    }
    else if (event == data) {
      outputter = size > 0 ? local_pipe : thread_pipe;
      input.read(outputter);
      // exactly like process
      continue_seq_using_buffers(pkt);
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
  void read(outputter);
  void finish();
  // possibly others
};

// ///////////// //
// Process Stage //
// ///////////// //

class stage {
  void data(pkt, outputter) = 0;
  // propogating events is fullfilled by the stage sequence
  void abandon() = 0;
  void flush() = 0;
  void finish() = 0;
};

class process_stage : non_input_stage {
};

// //////////// //
// Output Stage //
// //////////// //

// this organisation means we don't need to deletgate -- there's only one
// virtual call for each of the stage methods and two for the data method.
//
class output_stage_base : stage {
  void output(pkt, outputter) = 0;
  void reconfigure(pkt) = 0;

  // a pipe to the input thread
  outputter input_events_;

  void data(pkt, outputter) {
    // fullfills the 'reqconfigure' event.  Somebody has to check it.  If we do
    // it here then it avoids type checking of all stages.

    if (pkt.configuration != last_configuration) {
      this->reconfigure(pkt);
    }

    this->output(pkt, input_events_);
    outputter.write(pkt);
  }
};

class output_stage : output_stage_base {
};

// ////////////// //
// Observer Stage //
// ////////////// //

class observer_stage : stage {};

// ///////////////// //
// Terminator Stages //
// ///////////////// //

// start terminators are fullfilled by the stage_sequence

class terminator_stage : observer_stage {

};


