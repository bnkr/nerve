/*!
\file
\brief File access data.
*/

#ifndef FFMPEG_FILE_HPP_d7epgkb5
#define FFMPEG_FILE_HPP_d7epgkb5

namespace ffmpeg {

//! \ingroup grp_ffmpeg
//! Wrapper for AVFormatContext, involving all the opening/closing etc..
//!
//! See:
//! - http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVFormatContext.html
class file {
  public:
    //! \name Constructors/Destructors
    //@{

    //! Open the header and inspect the streams.
    file(const char * const file) {
      init(file);
    }

    //! The initialiser parameter forces you to initialise the library.
    file(const ffmpeg::initialiser &, const char * const file) {
      init(file);
    }

    ~file() {
      av_close_input_file(format_);
    }

    //@}

    //! \brief Print the format to stderr.  \c filename is merely informational.
    void dump(const char *filename = NULL) const {
      filename = (filename) ?: file_name();
      // TODO: what are those otehr parameters for?
      const int is_output = 0;
      const int index = 0; // I think this is the stream index...
      ::dump_format(format_, index, filename, is_output);
    }

    //! \name FFmpeg Accessors
    //@{

    //! \brief Return the ffmpeg struct.  Prefer the format accessor members.
    const AVFormatContext &av_format_context() const { return *format_; };
    AVFormatContext &av_format_context() { return *format_; }

    //@}

    //! \name Format accesors.
    //@{

    // Note: some of these are deduced from the stream context - never set them here.

    //! \brief How many streams in the file (some of which might be audio streams).
    std::size_t num_streams() const { return av_format_context().nb_streams; }
    //! \brief Or 0 if unknown.
    int64_t size() const { return av_format_context().file_size; }
    //! \brief In units of AV_TIME_BASE fractional seconds.  See \link calculate_duration() \endlink.
    int64_t duration() const { return av_format_context().duration; }
    //! \brief In AV_TIME_BASE fractional seconds.
    int64_t start_time() const { return av_format_context().start_time; }
    //! \brief Divide by 1000 to get kb/s of course.
    int bit_rate() const { return av_format_context().bit_rate; }
    //! \brief First byte of actual data.
    int data_offset() const { return av_format_context().data_offset; }
    //! \brief Size of the file minus the header.
    int64_t data_size() const { return size() - data_offset(); }

    //! Name of opened file.
    const char *file_name() const { return av_format_context().filename; }

    //! Alias for AV_TIME_BASE_Q.  All times in this structure are in units of
    //! this.  See av_rescale_q and the stream_time, file_time structures.
    AVRational time_base_q() const { return AV_TIME_BASE_Q; }

    //! Alias for AV_TIME_BASE.
    int64_t time_base() const { return AV_TIME_BASE; }

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

    //! \name Deprecated functions.
    //! \deprecated  Use size()
    //@{
    int64_t file_size() const FF_ATTRIBUTE_DEPRECATED { return size(); }
    const AVFormatContext &format_context() const FF_ATTRIBUTE_DEPRECATED { return av_format_context(); }
    AVFormatContext &format_context() FF_ATTRIBUTE_DEPRECATED { return av_format_context(); }
    void dump_format(const char *file = NULL) const FF_ATTRIBUTE_DEPRECATED { return dump(file); }
    //@}

  private:
    void init(const char *const file) {
      // I need a get_last_error message function, or at least some kind of errno?.

      // no docs somewhere...
      // TODO: I will need to use this later on.
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

  private:
    AVFormatContext *format_;
};


}

#endif
