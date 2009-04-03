/*!
\file
\brief C++ helpers for ffmpeg.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.

Ffmpeg is a pretty complicated library and this wrapper aims to simplify it as
well as add needed functionality like splitting the stream into packets for sdl
output.  In order to do this, the classes here wrap the ffmpeg structs and expose
a subset of their fields.  These are the ones you actually need, rather than
internal stuff.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

// TODO:
//   this implies that later on, the wrappers should be part of nerved instead
//   of the client.  Perhaps the client should be a completely seperate package?
#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

// FFmpeg developers hate C++, and not without good reason!  But then I have
// always been a masochist...
extern "C" {
#ifdef HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
#  include <ffmpeg/libavcodec/avcodec.h>
// Assume this to get a easier to diagnose error message.
#else /*if defined(HAVE_FFMPEG_AVCODEC_H)*/
#  include <ffmpeg/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
#  include <ffmpeg/libavformat/avformat.h>
#else /*if defined(HAVE_FFMPEG_AVFORMAT_H)*/
#  include <ffmpeg/avformat.h>
#endif
}

#include "ffmpeg/data.hpp"

#include "../include/aligned_memory.hpp"
#include <cstring>
#include <cstdlib>

namespace ffmpeg {

//! \brief Seperate stateful object which builds packets.
//! It must be seperate so it can exist in a higher scope than a file
//! and its audio stream.  This is to be used by the audio_decoder, and
//! should never need to be touched by the other code.
//
//TODO:
//  given that the state is seperate, I could now cause the audio_decoder
//  to immediately read the frame in ctor instead of doing it seperately.
//  This would make things safer to use.  Note: this changes rather a lot
//  wrt frame delaying.
class packet_state : boost::noncopyable {
  public:
    //! \brief The packet size is the size of the buffer to pass to the output queue.
    //! Silence value is normally 0, but sometimes it's not!
    packet_state(std::size_t packet_size, int silence_value)
    : packet_(NULL), packet_size_(packet_size), packet_index_(0), silence_value_(silence_value) {
      packet_ = std::malloc(packet_size);
      assert(packet_ != NULL);
    }

    ~packet_state() {
      std::free(packet_);
    }

    std::size_t index() const { return packet_index_; }
    std::size_t size() const { return packet_size_; }

    //! \brief Append the most possible; return how many appened.
    std::size_t append_max(void *buffer, std::size_t max_bytes)  {
      const std::size_t bytes_left = packet_size_ - packet_index_;
      const std::size_t copy_len = std::min(max_bytes, bytes_left);

      uint8_t *sample_buffer = ((uint8_t *) packet_) + packet_index_;
      std::memcpy(sample_buffer, buffer, copy_len);
      packet_index_ += copy_len;

      return copy_len;
    }

    //! \brief Set unused bytes to silence and return the ptr on the heap.
    void *get_final() {
      finalise();
      return clear();
    }

    // TODO:
    //   resize() could be necessary later.  It's a problem if the packet size is
    //   reduced though.  Forcing to clear first is a workaround, but not a very
    //   nice one... possibly it's a neceesary one though if you end up having
    //   to restart the audio thread anyway.

    //! \brief Returns old packet (on the heap).
    void *reset() {
      /// TODO:
      ///   need to use some kind of allocator to get these; also an RAII type.
      void *p = clear();
      assert(packet_ == NULL);
      assert(p != NULL);
      packet_ = std::malloc(packet_size_);
      // std::cout << "reset(): release: " << p << std::endl;
      // std::cout << "reset(): alloc:   " << packet_ << std::endl;
      return p;
    }

    //! \brief Like reset_packet(), but don't allocate a new one.
    void *clear() {
      // this shouldn't be called if we didn't finish it off already.
      assert(index() == size());
      void *p = packet_;
      packet_ = NULL;
      packet_index_ = 0;
      return p;
    }

    //! \brief For observation only!  Use the stateful functions.
    const void *ptr() { return packet_; }

  private:
    //! \brief Set the remainder of the buffer to silence
    void finalise() {
      std::memset(((uint8_t*)packet_ + packet_index_), silence_value_, packet_size_ - packet_index_);
      packet_index_ = packet_size_;
    }

    void *packet_;
    const std::size_t packet_size_;
    std::size_t packet_index_;
    const int silence_value_;
};

//! \brief Reads frames into buffers of a given size in a packet_state.
//! Loop on get_packet().  Remember to get any remaining data in packet_state when
//! completely done.
//TODO:
//  this should be split up very much and changes when we have the plugin
//  stage framework.  It should simply return the buffers; therefore the design
//  will be very different.  Including packet_state would not be needed - that
//  would be used in the output plugin.
class audio_decoder {
  public:
    //! \brief Buffer size is what should be given to the audio output.
    audio_decoder(packet_state &packet_state, ffmpeg::audio_stream &stream)
    : stream_(stream), buffer_index_(0), packet_(packet_state) {
      // my assumption is that it needs some N of 16 bit integers, even
      // though we actually just write random stuff to it.
      assert(total_buffer_bytes % sizeof(int16_t) == 0);
    }

    //! \deprecated use decode(fr);
    void decode_frame(const ffmpeg::frame &fr) {
      decode(fr);
    }

    //! \brief Set the internal state for this new frame.
    void decode(const ffmpeg::frame &fr); // currently outline to reduce compiler time.
    void truncate_pre_silence(const int);
    void truncate_silence(const int);

    //! \brief Get the next packet_size sized buffer from the frame, or NULL if there isn't one.
    //TODO:
    //  return something like an auto_ptr of course; really it should be done via.
    //  an allocator or even some kind of visitor function?  That would be less
    //  error prone, certainly.  Ideally I want to put it in a queue which is
    //  a memory pool.
    //
    //  Later we will not need to chunk the files because the output plugin will do it.
    void *get_packet() {
      if (buffer_index_ < buffer_size_) {
        const std::size_t stream_available = buffer_size_ - buffer_index_;
        uint8_t *stream_buffer = ((uint8_t *) buffer_.ptr()) + buffer_index_;

        buffer_index_ += packet_.append_max(stream_buffer, stream_available);

        // We filled up a buffer
        if (packet_.index() == packet_.size()) {
          // trc("packet complete");
          void *p = packet_.reset();
          // std::cout << "get_packet(): " << p << std::endl;
          return p;
        }
        else {
          // trc("packet is incomplete");
          // we are not allowed to overrun the buffer.
          assert(packet_.index() < packet_.size());
          return NULL;
        }
      }
      else {
        // we must have actually outputted all the data
        assert(buffer_index_ == buffer_size_);
        // trc("we have reached the end of the buffer: ind = "  << buffer_index_ << " vs. size = " << buffer_size_);
        return NULL;
      }
    }

    //! \brief Get a packet with unfilled bytes set nul.  A new state is NOT allocated.
    // void *get_final_packet() {
    //   void *p = get_packet();
    //   if (p != NULL) {
    //     std::cerr << "get_final_packet(): error: something has gone horribly wrong -- there are packets left to output!" << std::endl;
    //   }

    //   packet_.finalise();
    //   return packet_.clear();
    // }


  private:
    int decode(int16_t *output, int *output_size, const uint8_t *input, int input_size) {
      return avcodec_decode_audio2(&stream_.codec_context(), output, output_size, input, input_size);
    }

    void reset_buffer() {
      buffer_index_ = 0;
      buffer_size_ = 0;
    }

    ffmpeg::audio_stream &stream_;

    // work out the aligned buffer type
    // Later we will alloc this from a pool and pass it down to the other plugs.
    // It must be a special type which carries its own dimensions and properties
    // about the audio it carries so we can automatically configure and so on.
    static const std::size_t total_buffer_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    static const std::size_t total_buffer_size = total_buffer_bytes / sizeof(int16_t);
    static const std::size_t alignment = 16;
    typedef aligned_memory<alignment, total_buffer_size, int16_t> buffer_type;

    buffer_type buffer_;

    // the amount of the buffer which was written to by ffmpeg (in bytes);
    std::size_t buffer_size_;
    // stateful data - where have we outputted this frame up to? (In bytes)
    std::size_t buffer_index_;

    // working state
    packet_state &packet_;
};


class decoded_block {

};


} // ns ffmpeg


#endif

