/*!
\file
\brief Reduce compile time.
*/

#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

#include "../../wrappers/ffmpeg.hpp"

#include <limits>
#include <queue>
#include <cassert>
#include <cstdlib>
#include <cmath>

//! Calculate whether a time is in a range.
class range_calculator {
  public:

    // TODO:
    //   We should rescale the activation time to be in the same units as the
    //   stream; ditto to max_time.  How does the plugin deal with it - I guess
    //   we need a lib call to the input plug to ask it to give us tine info.
    //   Bleh... virtual function I guess?

    double activation_seconds() const {
      return 1;
    }

    range_calculator(const ffmpeg::stream_time &time) : time_(time) {}

    bool end() const {
      double remaining = max_time_as_seconds() - time_as_seconds();
      return remaining > 0 && remaining < activation_seconds();
    }

    bool start() const {
      return time_as_seconds() < activation_seconds();
    }

    double time_as_seconds() const {
      return time_.seconds();
    }

    double max_time_as_seconds() const {
      return HUH;
    }

  private:
    const ffmpeg::stream_time &time_;
    HUH max_stream_time_;
};

//! Calculate the average of a bunch of samples and determine if there was an
//! abrupt change in the average.
class sample_calculator {
  public:
    static const std::size_t num_samples = HUH;

    //! \name Accessors
    //@{

    double tolerance() const {
      // a 50% change is abrupt...
      return std::numeric_limits<int16_t>::max() * 0.5;
    }

    double difference() const { return difference_; }

    //@}

    //! Calculation method.
    void new_average(HUH &samples, ...) {
      // hum... I could work out the best dropping position here
      last_average_ = average_;
      average_ = HUH(samples);
      difference_ = average_ - last_average_;
    }


    //! \name Tests
    //@{

    bool abrupt_quietness() const {
      return -difference() > tolerance();
    }

    bool abrupt_loudness() const {
      return difference() > tolerance();
    }

    bool abrupt_change() const {
      return std::abs(difference()) > tolerance();
    }

    //@}


  private:
    double difference_;
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

sample_calculator calc;
sample_buffer buffer;
algorithm_state algo;

void ffmpeg::audio_decoder::decode(ffmpeg::frame &fr) {
  reset_buffer();

  int used_buffer_size = buffer_type::byte_size;
  int used_bytes = decode(buffer_.ptr(), &used_buffer_size, &(fr.av_packet()));

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

loop:

  // TODO:
  //   what about if we have a really huge packet so only some of it is in
  //   range?
  ffmpeg::stream_time packet_time(stream_, fr.presentation_timestamp());
  range_calculator in_range(packet_time);

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

