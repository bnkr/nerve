/*!
\file
\brief Relating to the audio stream and codec.
*/

#ifndef FFMPEG_AUDIO_STREAM_HPP_zjyzscb6
#define FFMPEG_AUDIO_STREAM_HPP_zjyzscb6

namespace ffmpeg {

// seeking needs this
class stream_time;

//! \ingroup grp_ffmpeg
//!
//! Interface to the audio stream and codec data.
//!
//! See:
//! - codec_context
//! - AVStream: http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVStream.html
//! - AVCodec: http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVCodec.html
//!
// TODO:
//   There is an increasing necesity to get the stream by index.  I think it
//   might be better to make this object more light-weight and access the
//   codec/stream pointers by using the codec array.  Perhaps I should have a
//   stream_reference which audio_stream extends.
//
//   The main problem is that audio_stream does initialisation here, so if you
//   refer to a stream which is not initialised you could be in Trouble.
class audio_stream {
  public:
    //! \name Constructors/destructors
    //@{
    //! \brief Gets the n'th audio stream.  Throws stream_error or unsupported_codec_error.
    //TODO:
    //  it would be nicer if we can inspect all the streams and choose the best one.
    //  Perhaps I should pass a chooser function?
    audio_stream(ffmpeg::file &f, std::size_t stream_num = 0) : file_(f) {
      AVFormatContext &fmt_ctx = f.av_format_context();
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
      codec_ = avcodec_find_decoder(av_codec_context().codec_id);
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

      if (avcodec_open(&av_codec_context(), codec_) < 0) {
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

    ~audio_stream() { avcodec_close(&av_codec_context()); }
    //@}

    //! \name Accessors
    //@{
    const ffmpeg::file &file() const { return file_; }
    ffmpeg::file &file() { return file_; }
    //@}

    //! \name Deprecated Accessors
    //@{
    AVCodecContext &codec_context() FF_ATTRIBUTE_DEPRECATED { return av_codec_context(); }
    const AVCodecContext &codec_context() const FF_ATTRIBUTE_DEPRECATED { return av_codec_context(); }
    AVCodec &codec() FF_ATTRIBUTE_DEPRECATED { return av_codec(); }
    const AVCodec &codec() const FF_ATTRIBUTE_DEPRECATED { return av_codec(); }
    AVStream &stream() FF_ATTRIBUTE_DEPRECATED { return av_stream(); }
    const AVStream &stream() const FF_ATTRIBUTE_DEPRECATED { return av_stream(); }
    //@}

    //! \name FFmpeg structure accessors.
    //@{


    //! \brief Contains codec data amongst other things.
    //!
    //! See:
    //! - http://www.dranger.com/ffmpeg/data.html#AVStream
    //! - http://www.irisa.fr/texmex/people/dufouil/ffmpegdoxy/structAVStream.html
    AVStream &av_stream() { return *stream_; }
    const AVStream &av_stream() const { return *stream_; }

    //! \brief Direct accessor to the codec informational data.  Enormous struct.
    //! Use \link codec_context \endlink unless giving this to an ffmpeg function.
    AVCodecContext &av_codec_context() { return *stream_->codec; }
    const AVCodecContext &av_codec_context() const { return *stream_->codec; }

    //! \brief Mostly to with the actual en/de-coding process.
    AVCodec &av_codec() { return *codec_; }
    const AVCodec &av_codec() const { return *codec_; }
    //@}


    //! \name AVStream and AVCodec (not codec_context) accessors.
    //@{

    //! All timestamps of *this stream* (ie, frames) are encoded in this way.
    const AVRational &time_base_q() const {
      assert(av_cmp_q(av_stream().time_base, av_codec_context().time_base) == 0);
      return av_stream().time_base;
    }

    //! \brief time_base() as a double.
    const double time_base_double() const FF_ATTRIBUTE_DEPRECATED { return av_q2d(av_stream().time_base); }

    //! Number of frames in the stream, or 0 if not known.
    int16_t frames() const { return av_stream().nb_frames; }

    //! Base frames per second.
    const AVRational &frame_rate() { return av_stream().r_frame_rate; }

    //! Stream id in the file's format_context.
    int index() const { return av_stream().index; }

    //@}

    // should be member of the file class.
    typedef enum {seek_forward} seek_forward_type;
    typedef enum {seek_backward} seek_backward_type;

    //! \name Seeking
    //! Note that the times must be in *this stream's* time_base.
    //TODO:
    //  See other todos regarding crapness of stream_time method for ensuring
    //  times are right.  (Long story short, there can be multiple streams and
    //  this would accept times from them all).
    //@{

    //! Seek forward to a keyframe.
    void seek_keyframe(const stream_time &where, seek_forward_type) {
      seek(where, 0);
    }

    //! Seek backward to a keyframe.
    void seek_keyframe(const stream_time &where, seek_backward_type) {
      seek(where, AVSEEK_FLAG_BACKWARD);
    }

    //! Seek forward to any type of frame.
    void seek(const stream_time &where, seek_forward_type) {
      seek(where, AVSEEK_FLAG_ANY);
    }

    //! Seek backward to any type of frame.
    void seek(const stream_time &where, seek_backward_type) {
      seek(where, AVSEEK_FLAG_ANY|AVSEEK_FLAG_BACKWARD);
    }

    //! Seek forward to a keyframe by byte location.
    void seek_byte(int64_t byte, seek_forward_type) {
      assert(byte >= 0);
      seek(byte, AVSEEK_FLAG_BYTE|AVSEEK_FLAG_ANY);
    }

    //! Seek backwards to a keyframe by byte location.
    void seek_byte(int64_t byte, seek_backward_type) {
      assert(byte >= 0);
      seek(byte, AVSEEK_FLAG_BYTE|AVSEEK_FLAG_ANY|AVSEEK_FLAG_BACKWARD);
    }

    //! Seek forward to a keyframe by byte location.
    void seek_keyframe_byte(int64_t byte, seek_forward_type) {
      assert(byte >= 0);
      seek(byte, AVSEEK_FLAG_BYTE);
    }

    //! Seek backwards to a keyframe by byte location.
    void seek_keyframe_byte(int64_t byte, seek_backward_type) {
      assert(byte >= 0);
      seek(byte, AVSEEK_FLAG_BYTE|AVSEEK_FLAG_BACKWARD);
    }

    //@}

  private:
    //! Seek wrapper.
    void seek(int64_t where, int flags) {
      av_seek_frame(&(file().av_format_context()), av_stream().index, where, flags);
    }

    // This has to be separate because the class must be defined before this
    // function is defined.
    void seek(const stream_time &, int flags);

  private:
    ffmpeg::file &file_;
    AVStream *stream_;
    AVCodec *codec_;
};

//! \ingroup grp_ffmpeg
//!
//! Generalised timestamp in the context of a time scale.
//!
//! In general it is better to use stream_time and file_time.
//!
//! Time needs some explaination.  The following rules hold:
//!
//! \verbatim
//!       avcodec_timestamp =
//! AV_TIME_BASE * time_in_seconds
//!
//!        time_in_seconds =
//! AV_TIME_BASE_Q * avcodec_timestamp
//! \endverbatim
//!
//! This is because AV_TIME_BASE_Q is 1/AV_TIME_BASE.
//!
//! File stuff is always in AV_TIME_BASE*, whereas streams are in the
//! time_base*() given in their AVStream struct (which is wrapped by
//! audio_stream in this library).
class scaled_time {
  public:
    typedef int64_t stamp_type;

    stamp_type timestamp() const { return timestamp_; }
    const AVRational &time_base() const { return base_; }

    //! Alter the timestamp into a new time_base.
    void rescale(const AVRational &to_base) {
      timestamp_ = timestamp_rescaled(to_base);
      base_ = to_base;
    }

    //! timestamp() * time_base(), which is basically just timestamp() * 1/frequency.
    double seconds() const { return timestamp() * av_q2d(time_base()); }

    //! A conversion aid.
    stamp_type timestamp_rescaled(const AVRational &to_base) const {
      return av_rescale_q(timestamp(), base_, to_base);
    }

  protected:
    scaled_time(const AVRational &base) : base_(base) {}
    scaled_time(const AVRational &base, stamp_type ts) : base_(base), timestamp_(ts) {}

  private:
    AVRational base_;
    stamp_type timestamp_;
};

//! \ingroup grp_ffmpeg
//!
//! A timestamp in units of a particular stream.
//TODO:
//  This is problematic because the idea of stream_time was to forbid passing a
//  dodgy time to one of the seek functions.  Since there can be multiple
//  audio_stream objects, this is a rather brittle method.  It might be better
//  to simply always call the rescaling function.
class stream_time : public scaled_time {
  public:
    //! Where t *must be* in s.time_base()
    stream_time(const ffmpeg::audio_stream &s, int64_t t)
    : scaled_time(s.time_base_q(), t) {}

    //! Rescaling constructor
    stream_time(const ffmpeg::audio_stream &s, const ffmpeg::scaled_time &t)
    : scaled_time(s.time_base_q(), t.timestamp_rescaled(s.time_base_q())) { }
};

//! \ingroup grp_ffmpeg
//!
//! A timestamp in units of the global ffmpeg time base which is used by the
//! file.
class file_time : public scaled_time {
  public:
    //! Where t *must be* in f.time_base()
    file_time(const ffmpeg::file &f, int64_t t)
    : scaled_time(f.time_base_q(), t) {}

    //! Rescaling constructor
    file_time(const ffmpeg::file &f, const ffmpeg::scaled_time &t)
    : scaled_time(f.time_base_q(), t.timestamp_rescaled(f.time_base_q())) { }
};

inline void audio_stream::seek(const stream_time &st, int flags) {
  av_seek_frame(&(file().av_format_context()), av_stream().index, st.timestamp(), flags);
  // TODO: errors?
}

//! \ingroup grp_ffmpeg
//! Friendly interface for the AVCodecContext struct.
//!
//! This is *lots* of useful information like bitrate, sample rate, etc.
//!
//! See:
//! - http://www.dranger.com/ffmpeg/data.html#AVCodecContext
//! - http://www.irisa.fr/texmex/people/dufouil/ffmpegdoxy/structAVCodecContext.html
class codec_context {
  public:
    codec_context(audio_stream &str) : st_(str) { }

    int sample_rate() const { return ctx().sample_rate; }
    int channels() const { return ctx().channels; }
    int average_bit_rate() { return ctx().bit_rate; }

    //! Fractional seconds which all times are represented in.
    //!
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

    AVCodecContext &ctx() { return st_.av_codec_context(); }
    const AVCodecContext &ctx() const { return st_.av_codec_context(); }
};


} // ns ffmpeg

#endif
