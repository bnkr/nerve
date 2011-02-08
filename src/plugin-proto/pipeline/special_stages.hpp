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

