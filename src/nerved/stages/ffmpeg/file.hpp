// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_FFMPEG_HPP_mb2e2uor
#define STAGES_FFMPEG_HPP_mb2e2uor

#include "avlibs.hpp"

namespace ffmpeg {
  //! \ingroup grp_ffmpeg
  //! Human-readable duration.
  struct human_duration {
    // TODO:
    //   Can has boost.time?
    int hours;
    int mins;
    int secs;
    int ms;
  };

  //! \ingroup grp_ffmpeg
  //! Primary entry structure.  Opens a stream of some description.
  class file {
    public:
    typedef ffmpeg::human_duration human_duration_type;

    //! \name Constructors/Destructors
    //@{

    //! Open the header and inspect the streams.
    file(const char * const file);
    ~file() { ::av_close_input_file(format_); }

    //@}

    //! \name FFmpeg Accessors
    //@{

    //! \brief Return the ffmpeg struct.  Prefer the format accessor members.
    const AVFormatContext *av_format_context() const { return format_; };
    AVFormatContext *av_format_context() { return format_; }

    //@}

    //! \name Time
    //@{

    // Note: some of these are deduced from the stream context - never set them here.

    //! In units of AV_TIME_BASE fractional seconds.  See \link calculate_duration() \endlink.
    int64_t duration() const { return av_format_context()->duration; }

    //! Calculate duration into the correct units.
    human_duration_type human_durtion() const;

    //! In AV_TIME_BASE fractional seconds.
    int64_t start_time() const { return av_format_context()->start_time; }

    //! Alias for AV_TIME_BASE_Q.  All times in this structure are in units of
    //! this.  See av_rescale_q and the stream_time, file_time structures.
    AVRational time_base_q() const { return AV_TIME_BASE_Q; }

    //! Alias for AV_TIME_BASE.
    int64_t time_base() const { return AV_TIME_BASE; }

    //@}

    //! \name File Properties
    //@{

    //! Or 0 if unknown.
    int64_t size() const { return av_format_context()->file_size; }
    //! First byte of actual data.
    int data_offset() const { return av_format_context()->data_offset; }
    //! Size of the file minus the header.
    int64_t data_size() const { return size() - data_offset(); }
    //! Name of opened file.
    const char *file_name() const { return av_format_context()->filename; }

    //@}

    //! \name File Contents Properties
    //@{

    //! How many streams in the file (some of which might be audio streams).
    std::size_t num_streams() const { return this->av_format_context()->nb_streams; }
    //! Divide by 1000 to get kb/s of course.
    int bit_rate() const { return av_format_context()->bit_rate; }

    //@}

    //! Print the format to stderr.
    void dump();

    private:
    AVFormatContext *format_;
  };
}
#endif
