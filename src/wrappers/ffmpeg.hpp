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
    //TODO:
    //  *Might* be better to init a codec object based on an audio_stream; depends
    //  how/where the codec is used and how much stuff I have to wrap.  I think it's
    //  only used in
    AVCodec &codec() { return *codec_; }
    const AVCodec &codec() const { return *codec_; }

    //@}

    //! \brief All timestamps of *this stream* (ie, frames) are encoded in this way.
    //! Use timestamp * av_q2d(time_base()) on packet timestamps etc.  This is not
    //! the same as the codec time base, or AV_TIME_BASE which is used for the file
    //! context.
    const AVRational &time_base() const { return stream().time_base; }

    //! \brief time_base() as a double.
    const double time_base_double() const { return av_q2d(stream().time_base); }


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
//TODO: should probably take the
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
        // We can keep going, but there will be output errors.
        // TODO: more detail.
        std::cerr << "warning: less bytes read from the stream than were available." << std::endl;
      }

      if (used_buffer_size <= 0) {
        // TODO: more detail.
        std::cerr <<  "warning: nothing to decode." << std::endl;
      }
      else {
        // don't set this unless we know it's unsigned.
        buffer_size_ = (std::size_t) used_buffer_size;

        //
        // TODO:
        //   Also I should only do it on the last frame of a file.
        //
        // TODO:
        //   Source data/research:
        //
        //   If wikipedia is to be believed:
        //     "Encoder/decoder overall delay is not defined, which means there is no
        //     official provision for gapless playback. However, some encoders such as
        //     LAME can attach additional metadata that will allow players that can handle
        //     it to deliver seamless playback."
        //
        //   It later implies that you can use silence detection to get rid of
        //   silence.  http://en.wikipedia.org/wiki/Gapless_playback
        //
        //   This has stuff about timestamps:
        //   - http://www.dranger.com/ffmpeg/tutorial05.html
        //
        //   By looking at sweep we can see that there are gaps at the end *and* begining of
        //   an mp3.

        // TODO:
        //   the ultimate task is to;
        //   - determine where we are in the *stream*
        //     - we must use a time value because bytes will be a different timespan
        //       when at different bitrates.
        //   - if we are past a certain limit or before a certain limit
        //   - truncate the buffer where the end is lower than a certain threshhold
        //
        //   additionally:
        //   - make it work over multiple frames, eg if the last frame is exteremly
        //     small then we need to kill a longer gap.  Error case:
        //     - the limit dictates that two frames from the end of the song are subject
        //       to gap removal.
        //     - the entire second to last frame is empty
        //     - the last frame is not empty.


        {
          // this seems to be right.
          // double fractional_seconds = fr.presentation_time() * av_q2d(stream_.time_base());
          // int sec = (int) fractional_seconds;
          // int us = (fractional_seconds - sec) * 1000;

          // TODO:
          //   ok this sort of works, but not really.  There's still a click and I can't
          //   work out why.
          //
          //   When I use extreme values it messes the stream completely.  I have no idea
          //   why that happens at all.  Maybe there is some other issue?  Gah, what  I
          //   really need is a file output so I can analyse the waveform I generated.
          //
          //   Argh this is so weird.  If you drop a lot of frames  at the start, it kills
          //   the entire stream!  Is my buffer handling wrong or something?  Does it become
          //   the wrong number of bytes?
          // if (fractional_seconds >= 2 || fractional_seconds <= 0.5 ) {
          //   trc("drop this frame: " << fractional_seconds);
          //   buffer_size_ = 0;
          //   return;
          // }
        }


        // TODO:
        //   What I need to do here as a proof-of-concept is to determine exactly
        //   where I am in the stream and then drop everything from a place where
        //   I know I should.  Then I can `work backwards' so to speak and see what
        //   I need to remove.
        //
        //   For the primus pt1, sweep shows a clear period of silence somewhat
        //   after 0m02s.192.  Unfortunately the resolution is not clear enough to
        //   see precisely what time the cut is at.
        //
        //   The weird thing is that ffmpeg appears to put the track size at 02.195.
        //   while sweep sees it at 02.220.  Perhaps ffmpeg is doing some work of
        //   its own?

        // TODO:
        //   this will be a problem if we're not using signed 16bit I guess.
        //
        trc("frame: " << codec_context(stream_).frame_number() << " with " << fr.size() << " bytes (data offset = " << fr.file().data_offset() << ").");
        static int trimmed = 0;

        // Note: test properties.
        //
        // This ensures that pathalogical cases where a pure period of silence at the start or
        // end is removed.
        //
        // - p1 nogap last sample               = -19870
        // - p1 gap last sample                 = -19870
        // - p2 nogap first sample should be    = -20313
        // - p2 gap first sample should be      = -20313
        // - bytes of silence at end of p1      = 14688 (b274 to ebd4 (past eof) = 45684 to 60372) = 60372 - 45684
        //                                      = start on frame 28
        // - bytes of silence at start of p2    = 14688 (02c to 398c = 44 to 14732) = 14732 - 44
        //                                      = stop on frame 4
        //
        // Looks like it overlapped by one byte... really those last bytes should be
        // different.  Well... it doesn't matter much, it just means we don't do the
        // short crossfade strategy (or if we do it's the pathalogical case).
        // Crossfade comes later anyway.  We can experiment.
        //
        // acording to the complete file:
        // - the last bytes of the p1    = ???
        // - the initial bytes of the p2 = ???
        //
        // ps: remember endianness, remember stereo files have double samples,
        // remember .wav files have a variable length header.

        // TODO:
        //   need a dump of a mp3 which *does not* support the lame header, ie I
        //   need a raw dump of two mp3s concatinated.  I think I can do this using
        //   the file output.  I need it to see what the threshold should be for
        //   silence in a real MP3.

        // Note: keep this pathalogical case lying around.  Requirement is that
        // frames are 4096 bytes big (unless they are the end) otherwise the frame
        // counts won't match up.
        if (codec_context(stream_).frame_number() >= 28 && trimmed == 0) {
          truncate_silence<int16_t>(0);
          if (codec_context(stream_).frame_number() == 31) {
            trimmed = 1;
          }
        }
        else if (trimmed == 1 && codec_context(stream_).frame_number() <= 4) {
          // this one must work forward!
          truncate_pre_silence<int16_t>(0);
          if (codec_context(stream_).frame_number() == 4) {
            trimmed = 2;
          }
        }

        // note: optimal algorithm:
        //
        // This is really tricky because we have to hold up the pipeline.  This algorithm
        // is very similar for a internal gap killer.
        //
        // Algorithm:
        //
        //   finished killing = false
        //
        //   every frame:
        //
        //   if near enoguh to the end
        //     look for first non-silence from the end
        //
        //     if frame is partial
        //       flush frame buffers (they are not part of the last period of silence)
        //       flag partialnes (store the `real' dimensions of the frame)
        //       delay frame
        //     else if frame is empty
        //       delay frame
        //     end
        //   // if there was no silence period at the end of the last frame, we assume
        //   // that any silence at the start here is intended.
        //   // This property might not be desireable, eg to gapkill a wav to mp3 transition.
        //   else if near enough to the the start && frames are delayed
        //     search for first non-silence from the start
        //
        //     // this bit is wrong. - we want to just drop empty frames.
        //     if frame is partial
        //       // we reached the definite end of the silence period
        //       delete empty frames
        //       flush previous partial frame with the correct dimensions
        //       flush this partial frame with the correct dimensions
        //     else frame is empty
        //       // what happens to these frames?
        //       delay frame
        //     end
        //   //
        //   else if frames are delayed
        //     flush
        //   end
        //

        // note: methods of delaying frames
        //
        // - copy into a giant buffer
        //   - memcpy is slow
        //   - completely dropping frames is free
        //   - might end up doing reallocs for the first few times.
        //   - hard to mempool
        // - allocate this frame buffer (the aligned_memory) and put it
        //   on a queue.
        //   - requires a queue which could be and is definitely using
        //     allocs.
        //   - easier to pool (except the aligned memory bit is rather
        //     wastefull - better to align the base address and dispatch
        //     in units of $alignment - would that be possible for frame
        //     buffers?  They might not all be %16 anyway?
        //   - frame buffers are bloody huge 192000 bytes.
        //
        // Gr... I might be better off trying both and profiling it.
        //
        //
      }
    }


    //! \brief Truncate the beginning of the bufgfer if it starts with silence
    template <class UnitsOf>
    void truncate_pre_silence(const int threshold) {
      trc("started with a buffer of " << buffer_size_ << " bytes");
      std::size_t elts = buffer_size_ / sizeof(UnitsOf);
      // trc("there are " << elts <<  " of that unit in the array");
      std::size_t num_trimmed = 0;
      UnitsOf *samples = (UnitsOf*) buffer_.ptr();
      for (int i = 0; i < elts; ++i) {
        // trc(i);
        // if (samples[i] >= (-threshold) && samples[i] <= threshold) {
        if (samples[i] == 0) {
          buffer_index_ += sizeof(UnitsOf);
          num_trimmed++;
        }
        else {
          trc("Found a byte " << samples[i]);
          break;
        }
      }

      if (! num_trimmed) {
        trc("nothing truncated");
        return;
      }

      int16_t first_byte = samples[buffer_index_ / sizeof(UnitsOf)];
      trc("final byte is: " << first_byte);
      trc("index has become " << buffer_index_ <<  " bytes.");
      trc("truncated to " << buffer_size_ <<  " bytes.");
      trc("trimmed      " << num_trimmed  << " times.");
    }

    //! \brief Truncate the buffer if it ends with silence.
    template <class UnitsOf>
    void truncate_silence(const int threshold) {

      // TODO:
      //   aaaarrgh!!!  I've just realised why this doesn't work.  It's *signed*
      //   data so it should be
      //
      //     (samples[i] >= -threshold && samples[i] <= threshhold)
      trc("started with a buffer of " << buffer_size_ << " bytes");
      std::size_t elts = buffer_size_ / sizeof(UnitsOf);
      // trc("there are " << elts <<  " of that unit in the array");
      std::size_t num_trimmed = 0;
      UnitsOf *samples = (UnitsOf*) buffer_.ptr();
      for (int i = elts - 1; i >= 0; --i) {
        // trc(i);
        // if (samples[i] >= (-threshold) && samples[i] <= threshold) {
        if (samples[i] == 0) {
          buffer_size_ -= sizeof(UnitsOf);
          num_trimmed++;
        }
        else {
          trc("Found a byte " << samples[i]);
          break;
        }
      }

      if (! num_trimmed) {
        trc("nothing truncated");
        return;
      }


      int16_t final_byte = samples[buffer_size_ / sizeof(UnitsOf) - 1];
      trc("final byte is: " << final_byte);
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

