/*!
\file
\brief C++ helpers for ffmpeg.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

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

#include <stdexcept>
#include <cassert>
#include <iostream> // debug

namespace ffmpeg {

//! \brief Base class for errors.
struct ffmpeg_error : public std::runtime_error {
  // TODO: possibility for a getLastError version?
  ffmpeg_error(const std::string &msg) : runtime_error(msg) {}
};

#define BASIC_EXCEPTION(name__)\
  struct name__ : public ffmpeg_error {\
    name__(const std::string &msg) : ffmpeg_error(msg) {}\
  }

// TODO: these should be expanded to carry more error information
BASIC_EXCEPTION(file_error);
BASIC_EXCEPTION(stream_error);
BASIC_EXCEPTION(codec_open_error);
BASIC_EXCEPTION(unsupported_codec_error);

#undef BASIC_EXCEPTION

//! \brief Initialise the library and interface to the static parts.
class initialiser {
  public:
    initialiser(int log_level = AV_LOG_WARNING) {
      av_log_set_level(AV_LOG_DEBUG);
      av_register_all();
      //typedef void(*callback_type)(void *, int, const char *, va_list);
      //av_log_set_callback(test_loading_log_cb);
      // log callback stuff here.
    }
};

//! \brief Wrapper for AVFormatContext.
//TODO: maybe should take the initialiser?  But it needs a better name...
class file {
  public:
    // \brief Open the header and inspect the streams.
    file(const char * const file) {
      // I need a get_last_error message function, or at least some kind of errno?.

      // no docs somewhere
      AVInputFormat *const forced_format = NULL;
      // http://www.dranger.com/ffmpeg/data.html#AVFormatParameters
      AVFormatParameters *const parameters = NULL;
      // what is this buffer size for?
      const std::size_t buffer_size = 0;

      int ret = av_open_input_file(&format_, file, forced_format, buffer_size, parameters);
      if (ret != 0) {
        // TODO: why?  Have to do some of my own validation here I guess.
        throw file_error("couldn't open file");
      }
      assert(format_ != NULL);

      ret = av_find_stream_info(format_);
      // TODO:
      //   this fails whenever I open one of my .avi with error code 22.  I can't work
      //   out what this means, and since the code is exactly the same as the tutorial
      //   it's a bit of a mystery.
      if (ret != 0) {
        std::cerr << "error num: " <<  ret << " becomes " << strerror(ret) << std::endl;
        // TODO: say why.
        //
        switch (ret) {
          // case AVERROR_UNKNOWN: // duplicated AVERROR_INVALIDDATA
            // std::cout << "AVERROR_UNKNOWN" << std::endl; break;
          case AVERROR_IO:
            std::cout << "AVERROR_IO" << std::endl; break;
          case AVERROR_NUMEXPECTED:
            std::cout << "AVERROR_NUMEXPECTED" << std::endl; break;
          case AVERROR_INVALIDDATA:
            std::cout << "AVERROR_INVALIDDATA" << std::endl; break;
          case AVERROR_NOMEM:
            std::cout << "AVERROR_NOMEM" << std::endl; break;
          case AVERROR_NOFMT:
            std::cout << "AVERROR_NOFMT" << std::endl; break;
          case AVERROR_NOTSUPP:
            std::cout << "AVERROR_NOTSUPP" << std::endl; break;
          case AVERROR_NOENT:
            std::cout << "AVERROR_NOENT" << std::endl; break;
          case AVERROR_EOF:
            std::cout << "AVERROR_EOF" << std::endl; break;
          case AVERROR_PATCHWELCOME:
            std::cout << "AVERROR_PATCHWELCOME" << std::endl; break;
          default:
            std::cout << "unexpected error code" << std::endl; break;
        }

        throw stream_error("could not find codec parameters");
      }
    }

    ~file() {
      av_close_input_file(format_);
    }

    //! \brief Print the format to stderr.  \c filename is merely informational.
    void dump_format(const char *filename = NULL) const {
      // TODO: what are those otehr parameters for?
      ::dump_format(format_, 0, filename, 0);
    }

// TODO:
//   it would be neater to wrap this in an object, then we can just pass that around.  Same
//   strategy could be used for some of the sdl objects.

    //! \brief Return the ffmpeg struct.
    const AVFormatContext &format_context() const { return *format_; };
    AVFormatContext &format_context() { return *format_; }

    std::size_t num_streams() const { return format_context().nb_streams; }

  private:
    AVFormatContext *format_;
};

//! \brief Interface to the audio stream and codec data.
class audio_stream {
  public:
    //! \brief Gets the n'th audio stream.  Throws stream_error or unsupported_codec_error.
    //TODO:
    //  it would be nicer if we can inspect all the streams and choose the best one.
    //  Perhaps I should pass a chooser function?
    audio_stream(file &f, std::size_t stream_num = 0) {
      AVFormatContext &fmt_ctx = f.format_context();
      for (std::size_t i = 0; i < f.num_streams(); ++i) {
        if (fmt_ctx.streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
          if (stream_num != 0) {
            --stream_num;
          }
          else {
            stream_ = fmt_ctx.streams[i];
            assert(stream_ != NULL);
            assert(stream_->codec != NULL);
            goto ok;
          }
        }
      }

      throw stream_error("no audio stream found");
ok:
      codec_ = avcodec_find_decoder(codec_context().codec_id);
      if (! codec_) {
        // TODO: say what the codec is.
        throw unsupported_codec_error("");
      }

#if 0
      // TODO: don't know what this implies (or if it's relevant to audio).
      // Inform the codec that we can handle truncated bitstreams -- i.e.,
      // bitstreams where frame boundaries can fall in the middle of packets
      if (codec_->capabilities & CODEC_CAP_TRUNCATED) {
        codec_context().flags |= CODEC_FLAG_TRUNCATED;
      }
#endif

      if (avcodec_open(&codec_context(), codec_) < 0) {
        // TODO: expanded info
        throw codec_open_error("could not initialise codec");
      }

#if 0
      // TODO: not sure if I need this - just for videos?
      // Hack to correct wrong frame rates that seem to be generated by some
      // codecs
      if (codec_context().frame_rate > 1000 && codec_context().frame_rate_base == 1) {
        codec_context().frame_rate_base = 1000;
      }
#endif
    }

    ~audio_stream() { avcodec_close(&codec_context()); }

    //! \brief Contains codec data amongst other things.
    //! See: http://www.dranger.com/ffmpeg/data.html#AVStream
    AVStream &stream() { return *stream_; }
    const AVStream &stream() const { return *stream_; }

    //! \brief Direct accessor to the codec informational data.  Enormous struct.
    // TODO:
    //   better to initialise an codec_context object based on a fully constructed
    //   audio_stream because that struct is too gigantic to wrap here.  Might be
    //   a good idea to initialise multiple subsets of this in order to modularise
    //   the data.
    //! http://www.dranger.com/ffmpeg/data.html#AVCodecContext
    AVCodecContext &codec_context() { return *stream_->codec; }
    const AVCodecContext &codec_context() const { return *stream_->codec; }

    //! \brief Mostly to with the actual en/de-coding process.
    //TODO:
    //  *Might* be better to init a codec object based on an audio_stream; depends
    //  how/where the codec is used and how much stuff I have to wrap.
    AVCodec &codec() { return *codec_; }
    const AVCodec &codec() const { return *codec_; }

  private:
    AVStream *stream_;
    AVCodec *codec_;
};

//! \brief Initialised by pulling a frame from a \link ffmpeg::file \endlink.
class frame {
  public:
    frame(ffmpeg::file &file) : file_(file) {
      int ret = av_read_frame(&file.format_context(), &packet_);
      finished_ = (ret != 0);
    }

    ~frame() { av_free_packet(&packet_); }

    //! Stream finished?
    bool finished() const { return finished_; }

    const uint8_t *data() const{ return packet_.data; }
    int size() const { return packet_.size; }


  private:
    ffmpeg::file &file_;
    AVPacket packet_;
    bool finished_;

};
}

#include "aligned_memory.hpp"
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
//  This would make things safer to use.
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
    }

    void *packet_;
    const std::size_t packet_size_;
    std::size_t packet_index_;
    const int silence_value_;
};

//! \brief Audio decoding context - reads frames into buffers of size n.
//! This is very stateful.  There will usually be data left over from
//! a frame, so this must not go out of scope until we are 100% finished
//! with the entire stream (at least).
//TODO:
//  could I make an object which makes this one a bit safer to use?
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
    void decode(const ffmpeg::frame &fr) {
      /*
      From the docs:

      For the input buffer we over allocate by FF_INPUT_BUFFER_PADDING_SIZE
      because optimised readers will read in longer bitlengths.  We never
      actually read data up to that length and the last byte must be zero
      (ffmpeg doesn't always do that).

      Output must be 16-byte alligned because SSE needs it.

      Input must be "at least 4 byte aligned".  Again ffmpeg doesn't always
      do it in fr.data().

      Finally, the output must be at least AVCODEC_MAX_AUDIO_FRAME_SIZE.

      TODO:
        it seems weird that ffmpeg's own data is not OK to put directly
        into the decoder.
      */
      reset_buffer();

      int used_buffer_size = buffer_type::byte_size;
      int used_bytes = decode(buffer_.ptr(), &used_buffer_size, fr.data(), fr.size());

      if (used_bytes < fr.size())  {
        // what do we do?  Can it even happen?
      }

      if (used_buffer_size <= 0) {
        // TODO: more detail
        std::cerr <<  "nothign to decode" << std::endl;
        // do what?  We must read another frame.
        // return false;
      }
      else {
        // don't set this unless we know it's unsigned.
        buffer_size_ = (std::size_t) used_buffer_size;

        // TODO:
        //   this will be a problem if we're not using signed 16bit I guess.
        //   It doesn't appear to work anyway.  There is still a slight pop;
        //   no real difference between this and the original.
        //
        //   Also I should only do it on the last frame of a file.
        //
        // TODO:
        //   Truncating in the middle is not good enough.  I must only truncate
        //   contiguous periods of silence.  Otherwise you get popping.
        truncate_silence<int16_t>();

        // TODO:
        //   This dies horribly - loads of white noise is outputted.
        // truncate_silence<uint8_t>();
      }
    }

    //! \brief Truncate the buffer if it ends with silence.
    template <class UnitsOf>
    void truncate_silence() {
      trc("started with a buffer of " << buffer_size_ << " bytes");
      std::size_t elts = buffer_size_ / sizeof(UnitsOf);
      trc("there are " << elts <<  " of that unit in the array");
      std::size_t num_trimmed = 0;
      UnitsOf *samples = (UnitsOf*) buffer_.ptr();
      for (int i = elts - 1; i >= 0; --i) {
        // trc(i);
        if (samples[i] == 0) {
          buffer_size_ -= sizeof(UnitsOf);
          num_trimmed++;
        }
        else {
          break;
        }
      }
      trc("truncated to " << buffer_size_ <<  " bytes.");
      trc("trimmed      " << num_trimmed  << " times.");
    }

    //! \brief Get the next packet_size sized buffer from the frame, or NULL if there isn't one.
    //TODO:
    //  return something like an auto_ptr of course; really it should be done via.
    //  an allocator or even some kind of visitor function?  That would be less
    //  error prone, certainly.  Ideally I want to put it in a queue which is
    //  a memory pool.
    void *get_packet() {
      if (buffer_index_ < buffer_size_) {
        const std::size_t stream_available = buffer_size_ - buffer_index_;
        uint8_t *stream_buffer = ((uint8_t *) buffer_.ptr()) + buffer_index_;

        buffer_index_ += packet_.append_max(stream_buffer, stream_available);

        // We filled up a buffer
        if (packet_.index() == packet_.size()) {
          void *p = packet_.reset();
          // std::cout << "get_packet(): " << p << std::endl;
          return p;
        }
        else {
          // we are not allowed to overrun the buffer.
          assert(packet_.index() < packet_.size());
          return NULL;
        }
      }
      else {
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
    static const std::size_t total_buffer_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    static const std::size_t total_buffer_size = total_buffer_bytes / sizeof(int16_t);
    static const std::size_t alignment = 16;
    typedef aligned_memory<alignment, total_buffer_size, int16_t> buffer_type;

    buffer_type buffer_;

    // the amount of the buffer which was written to by ffmpeg.
    std::size_t buffer_size_;
    // stateful data - where have we outputted this frame up to?
    std::size_t buffer_index_;

    // working state
    packet_state &packet_;
};


class decoded_block {

};


} // ns ffmpeg


#endif

