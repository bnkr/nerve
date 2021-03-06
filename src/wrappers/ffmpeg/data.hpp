/*!
\file
\brief Basic data structures including exceptions.
*/
#ifndef FFMPEG_DATA_HPP_41n0wqzg
#define FFMPEG_DATA_HPP_41n0wqzg

#include <stdexcept>
#include <cassert>
#include <iostream>

namespace ffmpeg {

//! \ingroup grp_ffmpeg
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

//! \ingroup grp_ffmpeg
//! \brief Initialise the library and interface to the static parts.
class initialiser {
  public:
    initialiser(int log_level = AV_LOG_WARNING) {
      av_log_set_level(log_level);
      av_register_all();
      //typedef void(*callback_type)(void *, int, const char *, va_list);
      //av_log_set_callback(test_loading_log_cb);
      // log callback stuff here.
    }
};

}

#endif
