/*!
\file
\brief Reduce compile time.
*/

#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

#include "../../wrappers/ffmpeg.hpp"

#include <queue>
#include <cassert>

//! Calculate whether a time is in a range.
class range_calculator {
  public:
    static const HUH activation = HUH;

    range_calculator(HUH &time);

    bool end();
    bool start();
};

//! Calculate the average of a bunch of samples and determine if there was an
//! abrupt change in the average.
class sample_calculator {
  public:
    static const std::size_t num_samples = HUH;
    static const double tolerance = HUH;

    void new_average(HUH &samples, ...);

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

    void delay(HUH samples);
    void flush();
    void flush_after(HUH pos);
    void drop_before(HUH pos);
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

class file_time {
};


class audio_stream {

  public:

};

void ffmpeg::audio_decoder::decode(const ffmpeg::frame &fr) {
  reset_buffer();

  int used_buffer_size = buffer_type::byte_size;
  int used_bytes = decode(buffer_.ptr(), &used_buffer_size, &(fr.packet()));

  if (used_bytes < 0) {
    std::cerr << "error frame" << std::endl;
  }

  assert(used_bytes == fr.size());

  if (used_buffer_size <= 0) {
    std::cerr <<  "warning: nothing to decode." << std::endl;
    return;
  }
  else {
    // don't set this unless we know it's unsigned.
    buffer_size_ = (std::size_t) used_buffer_size;
  }

  // TODO:
  //   it shouldn't be *that* much different from this when we have generic
  //   plugins... the difference is I can set the extents of the packet myself
  //   rather than realocating a new packet.
  //
  //   Maybe I should just hack this into the real nerve binary?
loop:
  range_calculator in_range(fr.time());

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
          buffer.flush();
        }
        buffer.delay(samples);
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
          HUH pos = backtrack_to_good_drop_point();
          buffer.drop_before(pos);
          buffer.flush_after(pos);
        }
        else {
          buffer.delay(samples);
        }
    }
  }
  else {
    // range expired.
    if (algo.state != disable) {
      buffer.flush();
      algo.state = disable;
    }
    output_verbatim(samples);
  }

  goto loop;
}



