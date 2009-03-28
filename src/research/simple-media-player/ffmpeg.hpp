/*!
\file
\brief C++ helpers for ffmpeg.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

#include <nerved_config.hpp>

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

      dump_format(file);

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

namespace ffmpeg {

//! \brief Audio decoding context - reads frames into buffers of size n.
//! This is very stateful.  There will usually be data left over from
//! a frame, so this must not go out of scope until we are 100% finished
//! with the entire stream (at least).
//TODO:
//  could I make an object which makes this one a bit safer to use?
class audio_decoder {
  public:
    //! \brief Buffer size is what should be given to the audio output.
    audio_decoder(ffmpeg::audio_stream &s, std::size_t packet_size)
    : stream_(s), buffer_index_(0), packet_(NULL), packet_size_(packet_size) {
      // my assumption is that it needs some N of 16 bit integers, even
      // though we actually just write random stuff to it.
      assert(buffer_bytes % sizeof(int16_t) == 0);
      reset_packet();
    }

    //! \brief Set the internal state for this new frame.
    void decode_frame(const ffmpeg::frame &fr) {
      reset_buffer();

      int buffer_size = buffer_type::byte_size;
      int used_bytes = decode(buffer_.ptr(), &buffer_size, fr.data(), fr.size());

      if (used_bytes < fr.size())  {
        // what do we do?  Can it even happen?
      }

      if (buffer_size <= 0) {
        // TODO: more detail
        std::cerr <<  "nothign to decode" << std::endl;
        // do what?  We must read another frame.
        // return false;
      }
      else {
        // don't set this unless we know it's unsigned.
        buffer_size_ = (std::size_t) buffer_size;
      }
    }

    //! \brief Get the next packet_size sized buffer from the frame, or NULL if there isn't one.
    //TODO:
    //  return something like an auto_ptr of course; really it should be done via.
    //  an allocator or even some kind of visitor function?  That would be less
    //  error prone, certainly.  Ideally I want to put it in a queue which is
    //  a memory pool.
    void *get_packet() {
      if (buffer_index_ < buffer_size_) {
        const std::size_t bytes_left = packet_size_ - packet_index_;
        const std::size_t stream_available = buffer_size_ - buffer_index_;
        const std::size_t copy_len = std::min(stream_available, bytes_left);

        uint8_t *sample_buffer = ((uint8_t *) packet_) + packet_index_;
        uint8_t *stream_buffer = ((uint8_t *) buffer_.ptr()) + buffer_index_;

        std::memcpy(sample_buffer, stream_buffer, copy_len);
        packet_index_ += copy_len;
        buffer_index_ += copy_len;

        // We filled up a buffer
        if (packet_index_ == packet_size_) {
          return reset_packet();
        }
      }
      return NULL;
    }

    //! \brief Get a packet with unfilled bytes set nul.  A new state is NOT allocated.
    void *get_final_packet() {
      void *p = get_packet();
      if (p != NULL) {
        std::cerr << "get_final_packet(): error: something has gone horribly wrong -- there are packets left to output!" << std::endl;
      }

      std::memset(((uint8_t*)packet_ + packet_index_), 0, packet_size_ - packet_index_);


      return clear_packet();
    }


  private:
    int decode(int16_t *output, int *output_size, const uint8_t *input, int input_size) {
      return avcodec_decode_audio2(&stream_.codec_context(), output, output_size, input, input_size);
    }

    //! \brief Returns old packet (on the heap).
    void *reset_packet() {
      /// TODO:
      ///   need to use some kind of allocator to get these; also an RAII type.
      void *p = packet_;
      packet_ = std::malloc(packet_size_);
      packet_index_ = 0;
      return p;
    }

    void reset_buffer() {
      buffer_index_ = 0;
      buffer_size_ = 0;
    }

    //! \brief Like reset_packet(), but don't allocate a new one.
    void *clear_packet() {
      void *p = packet_;
      packet_ = NULL;
      packet_index_ = 0;
      return p;
    }

    ffmpeg::audio_stream &stream_;

    // work out the aligned buffer type
    static const std::size_t buffer_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    static const std::size_t buffer_size = buffer_bytes / sizeof(int16_t);
    static const std::size_t alignment = 16;
    typedef aligned_memory<alignment, buffer_size, int16_t> buffer_type;

    buffer_type buffer_;

    // refering to the actually used bytes.
    std::size_t buffer_size_;
    std::size_t buffer_index_;

    // working state
    void *packet_;
    const std::size_t packet_size_;
    std::size_t packet_index_;
};


class decoded_block {

};


} // ns ffmpeg


#endif

