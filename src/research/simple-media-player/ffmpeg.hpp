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
      //av_log_set_callback(test_loading_log_cb);

      // no docs somewhere
      AVInputFormat *const forced_format = NULL;
      // http://www.dranger.com/ffmpeg/data.html#AVFormatParameters
      AVFormatParameters *const parameters = NULL;
      // what is the buffer size for?
      const std::size_t buffer_size = 0;
      if (av_open_input_file(&format_, file, forced_format, buffer_size, parameters) != 0) {
        // TODO: why?
        throw file_error("couldn't open file");
      }
      assert(format_ != NULL);

      if (av_find_stream_info(format_) != 0) {
        throw ffmpeg_error("av_find_stream_info");
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

    const uint8_t *data() { return packet_.data; }
    int size() { return packet_.size; }


  private:
    ffmpeg::file &file_;
    AVPacket packet_;
    bool finished_;

};

//! \brief Decodes audio based on the data context of the audio stream.
class audio_decoder {
  public:
    audio_decoder(ffmpeg::audio_stream &s)
    : stream_(s) {}

    // TODO:
    //   this is a really messy way to do it.  How can I make it so you don't have
    //   to always specify those buffers but still keep it reasonable to use.  I think
    //   I just need to restrict how it works; for example return a pointer to the
    //   output or something.  It all depends exactly what other stateful things I
    //   have to do, eg, when no data is found.
    //
    //! \brief All sizes are in *bytes* not num elts.
    int decode(int16_t *output, int *output_size, const uint8_t *input, int input_size) {
      return avcodec_decode_audio2(&stream_.codec_context(), output, output_size, input, input_size);
    }

  private:
    audio_stream &stream_;
};

} // ns ffmpeg


#endif

