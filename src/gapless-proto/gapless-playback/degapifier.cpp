#include "degapifier.hpp"
#include "portabdbg.hpp"

/*
General review of this algorithm:

Affected by transition to the plugin interface:
- buffering is pretty inefficiant.  In the real plugin this should be solbed by:
  - editing packet boundaries in place when we drop bits
  - buffering entire packets instead of their samples.
- the case/if thing is dodgy:
  - some duplicated code because of the case statement - it should be swapped
    with the if(range).
  - the whole thing is probably unnecessary because the algorithm_state object
    is redundant when we can pull multiple packets.
    - note: current design of plugins which requires that plugins can be either
      multi-threaded or not at runtime makes this very hard.
- the commit method is rather confusing.  In the plugin, it should instead
  actually perform the packet editing operations there.  We almost certainly need
  to keep it, or something like it.  Otherwise we do too many operations in the
  loop.
- the whole thing is rather complicated, and involves a lot of hacks to make
  get_packet working.
- could be nice to do some flushing on the transition between end range and start
  range.  It would mean we buffer less, and for less time.  Currently it is done
  for free because of commit() and get_packet()... it might be that we still have
  it when commit() does the pushing.

Enhancments:
- we should calculate an optimal point to drop packets in.  It should be in the
  last_average period, so it will always be in the current samples buffer.

Broken stuff:
- there is no solution to dealing with settings or, in particular, settings
  changes.
- the algorithm doesn't handle a huge packet where only part of it is in range.
  That is to say: he range calculation is based on the packet, not on the bytes.
  - this also means that the in_range calculation is redundant: it would be better
    to hoist those conditions.
- the end of the last song might end up being cut eroneously because the delay buffer
  is never cleared

Style and general enhancments.
- begin-end iteration is not the fastest.
    while (begin != end) do(*begin++);
  vs.
    while (--count) do(*begin++);
  Also, most of the time we are calculating the begin or end via. a count anyway.
  It would be better to have some overloads to orgainse buffers both ways, though
  we must be careful to ensure it does the same kind of stuff.

*/

#include <nerved_config.hpp>
#include "../../wrappers/ffmpeg.hpp"
#include "packet_state.hpp"

#include <cmath>
#include <cstddef>

#include <limits>
#include <vector>

//! Calculate whether a time is in a range.
class range_calculator {
  public:
    range_calculator(const ffmpeg::scaled_time &max_time, const ffmpeg::scaled_time &time)
    : time_(time), max_stream_time_(max_time) {
      uint32_t samps = (uint32_t) (44100 * activation_seconds());
      trc("period is " << activation_seconds() << "s which is " << samps << " samples (for 44.1khz rate).");
    }

    double activation_seconds() const {
      return 0.1;
    }

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
    const ffmpeg::scaled_time &time_;
    const ffmpeg::scaled_time &max_stream_time_;
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
    typedef sample_type* drop_point_type;

    //! Calculation method.
    //TODO:
    //  When we cross a packet boundary, this should not compute the average.
    //  We should delay the packet and recompute starting on that delayed
    //  packet.  This is a really hard thing to do, but it has to be done anyway
    //  because the drop point could well be in a previous packet.
    void new_average(sample_type *samples, sample_type *max) {
      last_average_ = average_;
      sample_type *samples_end = std::max(max, samples + num_samples());
      // abs() because we only care about *amplitude*, not frequency.  -n is
      // equivilent to n.
      average_ = std::abs(compute_average(samples, samples_end));
      assert(average_ <= (double) std::numeric_limits<int16_t>::max());
      assert(average_ >= (double) 0); //-std::numeric_limits<int16_t>::max());

      last_drop_point_ = drop_point_;
      drop_point_ = calculate_drop_point(samples, samples_end);

      difference_ = last_average_ - average_;

      assert((last_average_ > average_ && difference_ > 0) || (last_average_ <= average_ && difference_ <= 0));
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

    // TODO: this should be done at the same time as the average calculation.
    // TODO:
    //   also the design of the sample buffer changes so that we need a
    //   size_t for the drop point type.
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
    //TODO:
    //  What if the last_average is in another packet?
    drop_point_type drop_point() const { return last_drop_point_; }

    //! \name Tests
    //@{

    bool abrupt_quietness() const { return -difference() > tolerance(); }
    bool abrupt_loudness() const  {
      bool yes = difference() > tolerance();
      if (yes) {
        uint16_t la = ((int16_t) last_average_) & 0xFFFF,
                 a = ((int16_t) average_) & 0xFFFF;
        trc("abrupt loudness detected: " << last_average_ << " vs. " << average_ << " => "
            << std::hex << la << " vs. " << a << std::dec);
      }
      return yes;
    }
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

//! Controller for delayed frames.  Any samples in here will either be flushed
//! at a later time or dropped.  In the packet based version, this is much
//! easier (and faster) because we can just delay entire packets.
//!
//! The output range thing is a hack to let us do flush_before() in the buffer
//! state.  We need the range because we're only flushing part of the buffer and
//! then appending to it later.  In a packet plugin, we could do it a bit easier
//! (probably) by immediately flushing the delay buffer when calling
//! flush_before().
class delay_buffer {
  public:
    typedef std::vector<int16_t> buffer_type;

    typedef buffer_type::iterator buf_iterator;

    delay_buffer() : ranged_output_(false) {}

    //! Accessor for outputting.
    const buffer_type &buffer() const { return buffer_; }
    buffer_type &buffer() { return buffer_; }

    //! Get rid of everything delayed.
    void drop() { buffer().resize(0); }

    //! Add samples to be delayed.
    void append(const int16_t *start, const int16_t *end) {
      buffer().insert(buffer().end(), start, end);
    }

    //! Set the range as outputable.  This is for the hack of flushing only part
    //! of the buffer.
    void output_range(buf_iterator start, buf_iterator end) {
      trc("delay: output range " << &(*start)  << " - " << &(*end));
      output_begin_ = start;
      output_end_ = end;
      ranged_output_ = true;
    }

    //! Called by get packet to get rid of what's in the buffer if we've
    //! finished with it.
    void reset_output() {
      buffer().erase(output_begin(), output_end());
      ranged_output_ = false;
    }

    buf_iterator begin() { return buffer().begin(); }
    buf_iterator end() { return buffer().end(); }

    //! get_packet needs these.  Without get_packet, they are redundant because
    //! we can immediately flush the delay buffer at flush_*() rather than at
    //! the buffer_state#commit().
    buf_iterator output_begin() { return (ranged_output_) ? output_begin_ : begin(); }
    buf_iterator output_end() { return (ranged_output_) ? output_end_ : end(); }

  private:
    buffer_type::iterator output_begin_;
    buffer_type::iterator output_end_;

    //! Persistant state over multiple packets.
    buffer_type buffer_;
    bool ranged_output_;

};

//! This class is a hack to allow get_packet to be able to access the sample
//! buffer as well as the delay buffer.  In the real plugin, there will be no
//! get_packet() function (we push packets from the algorithm) so this will be
//! unncessary -- buffer_state will be able to make its own packets whenever
//! necessary.
class sample_buffer {
  public:
    typedef int16_t sample_type;

    sample_type *samples_;
    sample_type *output_begin_;
    sample_type *output_end_;

    std::size_t elements_;

    //! alias begin
    const sample_type *samples() const { return begin(); }
    //! *Total* samples in the buffer.
    std::size_t size() const { return elements_; }

    void reset(int16_t *samples, std::size_t elements) {
      samples_ = samples;
      elements_ = elements;
      output_begin_ = samples_;
      output_end_ = samples_ + elements_;
    }

    sample_type *begin() { return samples_; }
    const sample_type *begin() const { return samples_; }
    sample_type *end() { return samples_ + elements_; }
    const sample_type *end() const { return samples_ + elements_; }

    //! Set the range to be outputted.
    void output_range(sample_type *begin, sample_type *end) {
      trc("samples: output range " << (void*) begin  << " - " << (void*) end);
      output_begin_ = begin;
      output_end_ = end;
    }

    //! get_packet needs these  If output_begin() == output_end() then the delay
    //! buffer should not be flushed.
    sample_type *output_begin() { return output_begin_; }
    sample_type *output_end() { return output_end_; }
};

//! Buffer state affector for one run of the algorithm.  This deals with the
//! delay buffer and the current sample input.  By default, the entire buffer is
//! delayed.
//!
//! This class is fairly generic and tolerant of weird things like changing ones
//! mind on what operation to do ^^.  It will be important to test things like
//! that.
class buffer_state {
  public:
    delay_buffer &delay_;
    sample_buffer &samples_;

    const std::size_t sample_period_;

    //! Apply a meaning to the partition.
    enum operation {op_drop, op_delay, op_flush, op_flush_before} operation_;

    //! Where to drop before/flush after if operation_ = op_drop
    std::size_t partition_;

    //! Sample period is the number of samples per iteration.
    buffer_state(delay_buffer &delay, sample_buffer &samples, std::size_t sample_period)
    : delay_(delay), samples_(samples), sample_period_(sample_period) {
      operation_ = op_delay;
      partition_ = 0;
    }

    //! Set all before \c index (including the delay buffer) to be dropped.
    //! Everything after will be outputted.
    void drop_before(std::size_t index) {
      operation_ = op_drop;
      partition_ = index;
    }

    //! Mark the entire array and the delay buffer to be outputted.
    void flush_all() {
      operation_ = op_flush;
    }

    //! Entire buffer should be outputted.  This works trivially because the
    //! sample output range defaults to the entire buffer.
    void noop() { }

    //! Mark everything before the index to be flushed.  In the real plugin this
    //! would directly flush the delay buffer and mark the sample buffer as it
    //! is now.
    void flush_before(std::size_t index) {
      partition_ = index;
      operation_ = op_flush_before;
    }

    //! Apply the state changes to the delay_buffer.  get_packet can work out
    //! Whether to output the delay buffer if there as anything in the samples
    //! buffer to output.
    //!
    //! In a packet-push based plugin arch, this is probably unnecessary - we
    //! can mess with the delay buffer immediately.  It probably is more
    //! efficient to store the values on the sample buffer, though.
    //!
    //! It would be really good if there could also be a way to get rid of this
    //! case statement.  I guess I would just store the ranges directly.
    void commit() {
      switch (operation_) {
        case op_flush_before:
          // This method is actually still beneficial in the plugin mode because
          // otherwise we might end up creating multiple new packets to do the
          // flushing with.  It seems likely therefore, that the commit() method
          // will remain important.
          //
          // Instead of this, we would:
          // - flush the entire delay buffer now
          // - edit the dimensions of the current packet (trim it to
          //   samples.begin()..samples.begin()+partiion )
          // - copy the remainder of the flushed packet into a newly allocated
          //   one.

          trc("flushing before " << partition_ << "; delaying after");
          delay_.output_range(delay_.begin(), delay_.end());
          delay_.append(samples_.begin() + partition_, samples_.end());
          samples_.output_range(samples_.begin(), samples_.begin() + partition_);
          break;
        case op_flush:
          trc("flush the entire buffer");
          samples_.output_range(samples_.begin(), samples_.begin() + partition_);
          break;
        case op_delay:
          trc("delay entire buffer");
          delay_.append(samples_.begin(),  samples_.end());
          samples_.output_range(samples_.end(), samples_.end());
          break;
        case op_drop:
          trc("drop everything before " << partition_ << "; flushing after");
          delay_.drop();
          samples_.output_range(samples_.begin() + partition_, samples_.end());
          break;
        default:
          throw std::logic_error("this shouldn't be reached - bad id of operation");
      }
    }
};

//! Deals with the period of the sample buffer we are dealing with.
struct iteration_state {
  typedef int16_t sample_type;

  sample_buffer &samples_;
  const std::size_t period_;

  std::size_t index_;

  sample_type *sample_begin_;
  sample_type *sample_end_;

  //! avg_period = number of samples to average.
  iteration_state(sample_buffer &samples, std::size_t avg_period)
  : samples_(samples), period_(avg_period), index_(0) {
    sample_begin_ = samples.begin();
    sample_end_ = std::min(samples_.end(), sample_begin_ + period_);
  }

  void next() {
    sample_begin_ += period_;
    index_ = std::min(index_ + period_, samples().size());
    sample_end_ = std::min(samples().end(), sample_begin_ + period_);
  }

  //! Where are we in the sample buffer?
  std::size_t index() const { return index_; }

  sample_buffer &samples() { return samples_; }

  //! Average sample_begin()..sample_end()
  sample_type *period_begin() { return sample_begin_; }
  sample_type *period_end() { return sample_end_; }
};

enum state_id {disable, first, main, finished};

//! Simple state data which is necessary over multiple packets.  In a plugin
//! architecture this is probably unnecessary because you can just pull more
//! packets whenever you need them.
struct algorithm_state {
  enum state_id state;

  algorithm_state() : state(disable) {}
};

const unsigned int num_samples = 32;
sample_calculator calc(num_samples);
delay_buffer delays;
sample_buffer samples;
algorithm_state algo;

// NOTE:
//   Expected drop point at start should be 4514.

// for debugging, keep track of our position in the original stream.
std::size_t audio_offset = 0;

void degapifier::degapify(ffmpeg::decoded_audio &aud) {
  // isn't there a way to get this?  I'm sure there is...
  trc(" * degapify from audio offset: " << audio_offset);
  aud.dump(std::cout);
  samples.reset((int16_t*) aud.samples_begin(), aud.samples_size() / sizeof(int16_t));

  iteration_state iteration(samples, num_samples);

  ffmpeg::scaled_time packet_time = aud.presentation_time();
  ffmpeg::scaled_time duration = aud.file_duration();
  duration.rescale(packet_time.time_base());

  range_calculator in_range(duration, packet_time);

  buffer_state buffer(delays, samples, num_samples);

  wmassert(in_range.end() xor in_range.start(), "start range and end range should not overlap");

  if (in_range.end()) {
    trc("Packet is in the end range.");
  }
  else if (in_range.start()) {
    trc("Packet is in the start range.");
  }

  std::size_t num_iterations = 0;
  while (iteration.period_begin() != iteration.period_end()) {
    // TODO:
    //   I think this would be more simple if the case statement was the top level,
    //   and we if the in_range in each case.
    //
    //   This needs to be checked now we changed the whole buffer handling stuff.


    // TODO:
    //   What do we do if we get here and we haven't finished doing
    //   inrange.start() ?
    if (in_range.end()) {
      calc.new_average(iteration.period_begin(), iteration.period_end());
      switch (algo.state) {
        // First go is a noop because we must have two averages.
        case finished:
          // fallthrough
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
            // In the real plugin, it's not a huge issue because this function
            // can directly do the flush of the delay buffer and just change
            // the dimensions on the sample buffer.
            buffer.flush_before(iteration.index());
          }
          else {
            // Noop. Samples wil be delayed unless we flush or drop.
          }
      }
    }
    else if (in_range.start()) {
      calc.new_average(iteration.period_begin(), iteration.period_end());
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
          // keep flushing everything until teh range expires
          // TODO:
          //   there must be a quicker way to do this?  Perhaps not if we ever
          //   get round to calculating the range based on bytes, not entire
          //   packets.
          trc("we already reached the drop point: flush everything and end the algorithm");
          buffer.flush_all();
          goto finish;
        default:
          // If there is never any abrupt loudness, we time out eventually.
          if (calc.abrupt_loudness()) {
            trc("found the start of the real audio: drop and finish the algorithm");
            // TODO:
            //   we *must* use the calculator to find the optimal drop point.
            //   Otherwise the cut is in some totally random place.
            buffer.drop_before(iteration.index());
            algo.state = finished;
            goto finish;
          }
          else {
            // Noop.  Samples are expected to be delayed unless we say otherwise.
          }
      }
    }
    else {
      // range expired.
      if (algo.state != disable) {
        algo.state = disable;
      }
      buffer.flush_all();

      // don't iterate - head for the commit.
      goto finish;
    }

    iteration.next();
    ++num_iterations;
  }

finish:
  trc("Finish algorithm: did " << num_iterations << " complete iterations.");
  audio_offset += aud.samples_size();
  // Set stuff up for get_packet.
  buffer.commit();

  assert((delays.output_end() - delays.output_begin()) % sizeof(int16_t) == 0);
  assert((samples.output_end() - samples.output_begin()) % sizeof(int16_t) == 0);
}

typedef enum {part_none, part_delays, part_samples} part_type;

//! Yay more funky hacks.
struct get_packet_state {
  //! Which part of the output?  Disambuguate between dptr and sptr.
  part_type part;
  //! current pointer to delays
  delay_buffer::buf_iterator dptr;
  // curent pointer to samples
  int16_t *sptr;

  get_packet_state() {
    part = part_none;
  }

};

get_packet_state gps;

void *degapifier::get_packet() {
  trc("get packets");
  // We always either delay the entire buffer, drop some stuff, or flush the
  // output_* range.  In the first case, the output_* range is zero so we
  // never get here; in the second, the delays output range is always zero;
  // in the third, we have both ranges.  Therefore, we can trivially use both
  // the output ranges as we want.  (Note: we don't care about a partial
  // samples range output - that's all handled by the flush_before hacks in
  // the degapifying algorithm and delays buffer).
  //
  // Note also: these invariants may change over time, and that's why the
  // get_packet() method is shite compared to the packet-push method :)

  assert(samples.output_begin() <= samples.output_end());
  assert(delays.output_begin() <= delays.output_end());

  assert(state_.size() % sizeof(int16_t) == 0);
  assert(state_.index() % sizeof(int16_t) == 0);

  if (samples.output_begin() == samples.output_end()) {
    trc("no packets to get");
    gps.part = part_none;
    return NULL;
  }
  else {
    std::ptrdiff_t max_bytes;
    std::size_t appended_bytes;
    switch (gps.part) {
      case part_none:
        trc("begin getting:");
        trc("  delays = " << (void*) &(*delays.output_begin()) << ".." << (void*) &(*delays.output_end()));
        trc("  samples = " << (void*) &(*samples.output_begin()) << ".." << (void*) &(*samples.output_end()));
        gps.dptr = delays.output_begin();
        gps.part = part_delays;
        gps.sptr = samples.output_begin();
        // fallthrough
      case part_delays:
        // This would be much nicer...
        // gps.dptr = state_.fill(gps.dptr, delays.output_end());
        max_bytes = delays.output_end() - gps.dptr;
        appended_bytes = state_.append_max(&(*gps.dptr), max_bytes);
        trc("packet index: " << state_.index() << " / " << state_.size());
        trc("from delays append " << appended_bytes << " / " << max_bytes);
        assert(max_bytes % sizeof(int16_t) == 0);
        wmassert_eq(appended_bytes % sizeof(int16_t), 0, "samples should not be cut into bits");
        gps.dptr += appended_bytes / sizeof(int16_t);

        if (gps.dptr == delays.output_end()) {
          gps.part = part_samples;
        }

        // If we didn't fill a complete packet then we'll fallthrough and
        // continue filling from the sample buffer.
        if (state_.size() == state_.index()) {
          return state_.reset();
        }

        // fallthrough
      case part_samples:
        max_bytes = samples.output_end() - gps.sptr;
        appended_bytes = state_.append_max(gps.sptr, max_bytes);
        wmassert_eq(appended_bytes % sizeof(int16_t), 0, "samples should not be cut into bits");
        trc("from samples append " << appended_bytes << " / " << max_bytes);
        assert(max_bytes % sizeof(int16_t) == 0);
        gps.sptr += appended_bytes / sizeof(int16_t);

        if (gps.sptr == samples.output_end()) {
          gps.part = part_none;
        }

        if (state_.size() == state_.index()) {
          return state_.reset();
        }

        break;
      default:
        throw std::logic_error("what the fuck");
    }
  }
  gps.part = part_none;
  return NULL;
}



