/*!
\file
\brief Reduce compile time.
*/

#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

#include "../../wrappers/ffmpeg.hpp"

#include <queue>

//! Calculate whether a time is in a range.
class range_calculator {
  public:
    range_calculator(void);

    bool end();
    bool start();
};

//! Calculate the average of a bunch of samples and determine if there was an
//! abrupt change in the average.
class sample_calculator {
  public:
    static const std::size_t num_samples = ;

    // TODO: this should take multiple samples
    void new_average(void);

    bool abrupt_quietness();
    bool abrupt_loudness();
    bool abrupt_change();

    double average_;
    double last_average_;
};

//! Delay samples which might be gappy.
class sample_buffer {
  public:
    // obv. need a much  better way of buffering things
    std::queue<int16_t> buffer_;
};

enum state_id {disable, first, main};

//! Simple state data.
struct algorithm_state {
  enum state_id state;

  algorithm_state() : state(disable) {}
};

range_calculator in_range;
sample_calculator calc;
sample_buffer buffer;
algorithm_state algo;

void ffmpeg::audio_decoder::decode(const ffmpeg::frame &fr) {
loop:
  range_calculator in_range(packet.time());

  if (in_range.end()) {
    calc.new_average(samples);
    switch (algo.state) {
      // First go is a noop because we must have two averages.
      case disable:
        algo.state = first;
        break;
      case first:
        if (calc.abrupt_quietness()) algo.state = main;
        break;
      default:
        // A later abrupt transition indicates we started cutting too early.
        if (calc.abrupt_loudness()) {
          flush();
        }
        delay();
    }
  }
  else if (in_range.start()) {
    calc.new_average(samples);
    switch (algo.state) {
      case disable:
        algo.state = first;
        break;
      case first:
        algo.state = main;
        break;
      default:
        // If there is never any abrupt loudness, we time out eventually.
        if (calc.abrupt_loudness()) {
          // TODO:
          //   which object is responsible for this searching?  The buffer
          //   I guess?
          pos = backtrack_to_good_drop_point();
          drop_before(pos);
          flush_after(pos);
        }
        else {
          delay();
        }
    }
  }
  else {
    // range expired.
    algo.state = disable;

    output_verbatim();
  }

  goto loop;
}



