/*!
\file
\brief Data wrappers for ffmpeg
*/
#ifndef FFMPEG_DATA_HPP_41n0wqzg
#define FFMPEG_DATA_HPP_41n0wqzg

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

//! \brief Wrapper for AVFormatContext, involving all the opening/closing etc..
class file {
  public:
    //TODO: should take the initialiser?  But it needs a better name...
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

    //! \deprecated Use dump().
    void dump_format(const char *f = NULL) { dump(f); }

    //! \brief Print the format to stderr.  \c filename is merely informational.
    void dump(const char *filename = NULL) const {
      // TODO: what are those otehr parameters for?
      const int is_output = 0;
      const int index = 0; // I think this is the stream index...
      ::dump_format(format_, index, filename, is_output);
    }

    //! \brief Return the ffmpeg struct.  Prefer the format accessor members.
    const AVFormatContext &format_context() const { return *format_; };
    AVFormatContext &format_context() { return *format_; }

    //! \name Format accesors.
    //@{

    // Note: some of these are deduced from the stream context - never set them here.

    //! \brief How many streams in the file (some of which might be audio streams).
    std::size_t num_streams() const { return format_context().nb_streams; }
    //! \brief Or 0 if unknown.
    int64_t file_size() const { return format_context().file_size; }
    //! \brief In units of AV_TIME_BASE fractional seconds.  See \link calculate_duration() \endlink.
    int64_t duration() const { return format_context().duration; }
    //! \brief In AV_TIME_BASE fractional seconds.
    int64_t start_time() const { return format_context().start_time; }
    //! \brief Divide by 1000 to get kb/s of course.
    int bit_rate() const { return format_context().bit_rate; }
    //! \brief First byte of actual data.
    int data_offset() const { return format_context().data_offset; }

    //@}

    //! \brief Calculate duration into the correct units.
    void calculate_duration(int &hours, int &mins, int &secs, int &ms) const {
      secs = duration() / AV_TIME_BASE;
      int us = duration() % AV_TIME_BASE;
      mins = secs / 60;
      secs %= 60;
      hours = mins / 60;
      mins %= 60;
      ms = (100 * us) / AV_TIME_BASE;
    }

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

    //! \name ffmpeg structure accessors.
    //@{

    //! \brief Contains codec data amongst other things.
    //! See:
    //! - http://www.dranger.com/ffmpeg/data.html#AVStream
    //! - http://www.irisa.fr/texmex/people/dufouil/ffmpegdoxy/structAVStream.html
    AVStream &stream() { return *stream_; }
    const AVStream &stream() const { return *stream_; }

    //! \brief Direct accessor to the codec informational data.  Enormous struct.
    //! Use \link codec_context \endlink unless giving this to an ffmpeg function.
    AVCodecContext &codec_context() { return *stream_->codec; }
    const AVCodecContext &codec_context() const { return *stream_->codec; }

    //! \brief Mostly to with the actual en/de-coding process.
    AVCodec &codec() { return *codec_; }
    const AVCodec &codec() const { return *codec_; }
    //@}


    //! \name AVStream and AVCodec (not codec_context) accessors.
    //@{

    //! \brief All timestamps of *this stream* (ie, frames) are encoded in this way.
    //! Use timestamp * av_q2d(time_base()) on packet timestamps etc.  This is not
    //! the same as the codec time base, or AV_TIME_BASE which is used for the file
    //! context.
    const AVRational &time_base() const { return stream().time_base; }

    //! \brief time_base() as a double.
    const double time_base_double() const { return av_q2d(stream().time_base); }

    //@}


  private:
    AVStream *stream_;
    AVCodec *codec_;
};

//! \brief Friendly interface for the AVCodecContext struct.
//! See:
//! - http://www.dranger.com/ffmpeg/data.html#AVCodecContext
//! - http://www.irisa.fr/texmex/people/dufouil/ffmpegdoxy/structAVCodecContext.html
class codec_context {
  public:
    codec_context(audio_stream &str) : st_(str) { }

    int sample_rate() const { return ctx().sample_rate; }
    int channels() const { return ctx().channels; }
    int average_bit_rate() { return ctx().bit_rate; }
    //! \brief Fractional seconds which all times are represented in.
    //! For fixed-fps content, timebase should be 1/framerate and timestamp increments
    //! should be identically 1.
    //!
    //! Note: this does not appear to be the same as audio_stream::time_base().
    //TODO: what is this for if it is zero all the time?
    const AVRational &time_base() const { return ctx().time_base; }
    double time_base_double() const { return av_q2d(time_base()); }
    //! \brief Which frame did we just decode?
    int frame_number() const { return ctx().frame_number; }

  private:
    audio_stream &st_;

    AVCodecContext &ctx() { return st_.codec_context(); }
    const AVCodecContext &ctx() const { return st_.codec_context(); }
};

//! \brief Initialised by pulling a frame from a \link ffmpeg::file \endlink.
//! See:
//! - http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVPacket.html
//
//TODO:
//  For informatuonal reasons it would be nice to take the an audio stream.
//  It messes up the interface a bit, though.
//
//  Example:
//
//    print codec_context(fr.file()).frame_number());
//
//  Also time base.
class frame {
  public:
    frame(ffmpeg::file &file) : file_(file) {
      // TODO: formatContext stores an AVPacket.  Does that mean I am doing a useless copy here?
      int ret = av_read_frame(&file.format_context(), &packet_);
      finished_ = (ret != 0);
    }

    ~frame() { av_free_packet(&packet_); }

    //! \name Packet accessors
    //@{
    //! Stream finished?
    bool finished() const { return finished_; }

    //! \brief Data buffer in the frame packet.
    const uint8_t *data() const { return packet_.data; }
    int size() const { return packet_.size; }

    //! \brief Timestamp of when to output this in units of time_base.
    //! Note that sometiems the decode timestamp is different from the presentation one, but
    //! this does not happen in audio streams.
    //TODO:
    //  could be better to wrap this time stuff in an object like frame_time and another
    //  object file_time.
    int64_t presentation_time() const { return packet_.pts; }

    //! \brief Presentation time in fractional seconds in the stream's time base.
    double stream_time_double(const audio_stream &stream) const {
      return presentation_time() * av_q2d(stream.time_base());
    }

    //! \brief Time in AV_TIME_BASE units.
    //TODO: this does not seem to be right.  Test for frame number = 0 - maybe it's just an offset?
    // int64_t presentation_time() const { return stream_time_sec() * AV_TIME_BASE; }

    //! \brief Byte position in stream.
    int64_t position() const { return packet_.pos; }
    //@}

    //! \brief The file this frame was pulled from.
    ffmpeg::file &file() { return file_; }
    const ffmpeg::file &file() const { return file_; }

    //! \brief This also shows all the field purposes.
    std::ostream &dump(std::ostream &o, const char *pf = "") {
      std::cout << pf << "presentation ts:  " << packet_.pts << std::endl;
      std::cout << pf << "decompression ts: " << packet_.dts << std::endl;
      std::cout << pf << "data:             " << (void*) packet_.data << std::endl;
      int al;
      std::size_t input_buf = (std::size_t) packet_.data;
      if (input_buf % 16 == 0) {
        al = 16;
      }
      else if (input_buf % 8 == 0) {
        al = 8;
      }
      else if (input_buf % 4 == 0) {
        al = 4;
      }
      else if (input_buf % 2 == 0) {
        al = 2;
      }
      else if (input_buf % 1 == 0) {
        al = 1;
      }
      else {
        al = -1;
      }
      std::cout << pf << "data alignment:   " << al << std::endl;
      std::cout << pf << "size:             " << packet_.size << std::endl;
      std::cout << pf << "stream index:     " << packet_.stream_index << std::endl;
      std::cout << pf << "packet_ flag:      " << packet_.flags << " ("
        << ((packet_.flags == PKT_FLAG_KEY) ? "keyframe" : "non-keyframe") <<  ")" << std::endl;
      std::cout << pf << "presentation dur: " << packet_.duration << std::endl;
      std::cout << pf << "deallocator:      " << (void*) packet_.destruct << std::endl;
      std::cout << pf << "byte offset:      " << packet_.pos << std::endl;

      return o;
    }

    //! \brief Use accessor member functions if possible.
    const AVPacket &packet() const { return packet_; }

  private:
    ffmpeg::file &file_;
    AVPacket packet_;
    bool finished_;
};
}

#endif
