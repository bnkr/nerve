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

/*
General review of this algorithm:

- lots of stuff is calculated more than once (especially involving the
  buffering procedure.
- some duplicated code because of the case statement - it should be swapped
  with the if(range).
- buffering is pretty inefficiant.
- the drop point calculation is really confusing
- again, mostly due to the case/if order, the goto loop thing is confusing.
- writing back into the audio_decoder's sample buffer is a recepie for disaster
  (but it only happens because there's no other method);
- design of the sample buffer is really bad - in the real version we should
  have it store packets and edit them, then just control the GC-ing and the
  pushing to the next unit.
- there is no solution to dealing with settings or, in particular, settings
  changes.
- sample_buffer needs to interact with stuff better.  Especailly, there are
  loads of reaclaulations every time you need to work out any kind of position.
  This will be a lot easier when we're buffering entire packets.  Nostly this
  is a problem with sample_buffer vs. sample_calculator's representation of
  sample identifiers.
*/

//! Calculate whether a time is in a range.
class range_calculator {
  public:

    double activation_seconds() const {
      return 0.5;
    }

    range_calculator(const ffmpeg::stream_time &max_time, const ffmpeg::stream_time &time)
    : time_(time), max_stream_time_(max_time) {}

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
      return max_stream_time_.seconds();
    }

  private:
    const ffmpeg::stream_time &time_;
    const ffmpeg::stream_time &max_stream_time_;
};

//! Calculate the average of a bunch of samples and determine if there was an
//! abrupt change in the average.
class sample_calculator {
  public:
    sample_calculator(std::size_t num_samples)
    : num_samples_(num_samples) {
    }

    //! \name Accessors
    //@{

    double tolerance() const {
      // a 50% change is abrupt...
      return std::numeric_limits<int16_t>::max() * 0.5;
    }

    double difference() const { return difference_; }

    std::size_t num_samples() const { return num_samples_; }

    //@}

    typedef int16_t sample_type;
    typedef sample_type *drop_point_type;

    //! Calculation method.
    void new_average(sample_type *samples, sample_type *max) {
      last_average_ = average_;
      sample_type *samples_end = std::max(max, samples + num_samples());
      average_ = compute_average(samples, samples_end);
      // this is wrong: it should be the data point in the last_average_
      last_drop_point_ = drop_point_;
      drop_point_ = calculate_drop_point(samples, samples_end);
      difference_ = average_ - last_average_;
    }

    double compute_average(sample_type *begin, sample_type *end) const {
      double v = 0;
      std::size_t n = 0;
      while (begin != end) {
        v += *begin;
        ++begin;
        ++n;
      }

      return v / (double) n;
    }

    drop_point_type calculate_drop_point(sample_type *begin, sample_type *end) const {
      --end;
      sample_type *next = begin;
      ++next;
      sample_type biggest_diff = 0;
      sample_type *drop_point = begin;
      while (begin != end) {
        sample_type diff = std::abs(*begin - *next);
        if (diff > biggest_diff) {
          biggest_diff = diff;
          drop_point = next;
        }
        ++next;
        ++begin;
      }

      return drop_point;
    }

    //! Best point in the group of averages to drop at.
    drop_point_type drop_point() const { return last_drop_point_; }

    //! \name Tests
    //@{

    bool abrupt_quietness() const { return -difference() > tolerance(); }
    bool abrupt_loudness() const  { return  difference() > tolerance(); }
    bool abrupt_change() const    { return std::abs(difference()) > tolerance(); }

    //@}


  private:
    std::size_t num_samples_;
    double difference_;
    double average_;
    double last_average_;

    drop_point_type last_drop_point_;
    drop_point_type drop_point_;
};

//! Delay samples which might be gappy.
class sample_buffer {
  public:
    // obv. need a much  better way of buffering things.  In the real thing, I
    // expect it will be much better to delay entire packets.

    typedef std::vector<int16_t> buffer_type;
    buffer_type buffer_;

    void delay(const int16_t * const samples, std::size_t number) {
      buffer_.reserve(buffer_.size() + number);
      buffer_.insert(buffer_.end(), samples, samples + number);
    }

    const buffer_type &buffer() const { return buffer_; }

    //
    typedef buffer_type::size_type drop_point_type;

    void flush(void *dest) {
      flush_after(dest, 0);
    }

    void flush_after(void *dest, drop_point_type pos) {
      // big hax - in reality this is packet based and we can just edit the
      // existing packets.  However, for the purpose of this prototype we need
      // to put it all into the big buffer.
      //
      // So what really happens is that the dimensions of the boundary packet:
      //
      //   1   2       3   4
      //   xxx xxx|yyy yyy yyyyyy
      //
      // 1 is ignored.  The dimensions of 2 are changed.  3 and 4 are left as
      // they are.  Then we can send 2, 3, and 4 to the push function.
      buffer_type::value_type *start = &(buffer_[pos]);
      std::size_t byte_len = (buffer_.size() - pos) * sizeof(buffer_type::value_type);
      assert(byte_len < AVCODEC_MAX_AUDIO_FRAME_SIZE); //< this is a hack in itself
      std::memcpy(dest, start, byte_len);
    }

    void drop_before(drop_point_type pos) {
      // in the real plugin, we would gc delayed packets here.
    }
};

enum state_id {disable, first, main, finished};

//! Simple state data.
struct algorithm_state {
  enum state_id state;

  algorithm_state() : state(disable) {}
};

const unsigned int num_samples = 32;
sample_calculator calc(num_samples);
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

  int16_t *samples = buffer_.ptr();
  int16_t *samples_end = buffer_.ptr() + buffer_size_;
  std::size_t position_in_buffer = 0;

loop:

  // TODO:
  //   what about if we have a really huge packet so only some of it is in
  //   range?  That is, our range starts in the middle of the packet: we have to
  //   output the first few bytes of the packet.
  ffmpeg::stream_time packet_time(stream_, fr.presentation_timestamp());
  // this API makes no fucking sense.
  ffmpeg::file_time duration(stream_.file(), stream_.file().duration());
  ffmpeg::stream_time converted_duration(stream_, duration);
  range_calculator in_range(converted_duration, packet_time);

  // I think this would be more simple if the case statement was the top level,
  // and we if the in_range in each case.

  if (in_range.end()) {
    calc.new_average(samples, samples_end);
    switch (algo.state) {
      // First go is a noop because we must have two averages.
      case finished:
      case disable:
        algo.state = first;
        break;
      case first:
        if (calc.abrupt_quietness()) algo.state = main;
        break;
      default:
        // A later abrupt transition indicates we started cutting too early.
        // TODO:
        //   what about a gradual loudness?  Do we need calc.sustained_loudness() ?
        if (calc.abrupt_loudness()) {
          buffer.flush(buffer_.ptr());
        }
        else {
          // it makes no real sense to copy this data into the buffer - we already
          // have it in the decoder buffer.  In the real plugin we need to work on
          // a packet in place, and then delay the entire packet if necessary.
          buffer.delay(samples, (std::ptrdiff_t) (samples - std::max(samples + num_samples, samples_end)));
        }
    }
  }
  else if (in_range.start()) {
    calc.new_average(samples, samples_end);
    switch (algo.state) {
      case disable:
        algo.state = first;
        break;
      case first:
        // noop so we have two samples - this only happens on the first track
        // playing.
        algo.state = main;
        break;
      case finished:
        break;
      default:
        // If there is never any abrupt loudness, we time out eventually.
        if (calc.abrupt_loudness()) {
          // buffer.drop_before(calc.drop_point()); <- noop here
          // TODO: we should use the calculator to find the optimal drop point.
          std::size_t pos = position_in_buffer;
          buffer.flush_after(buffer_.ptr(), pos);
          algo.state = finished;
        }
        else {
          buffer.delay(samples, std::ptrdiff_t (samples - std::max(samples + num_samples, samples_end)));
        }
    }
  }
  else {
    // range expired.
    if (algo.state != disable) {
      buffer.flush(buffer_.ptr());
      algo.state = disable;
    }
    return;
  }

  samples += num_samples;
  position_in_buffer += num_samples;
  if (samples < samples_end) {
    goto loop;
  }
}

