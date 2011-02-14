// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef OUTPUT_LOGGER_HPP_1uj7g16h
#define OUTPUT_LOGGER_HPP_1uj7g16h

#include <cstdarg>
#include <boost/thread.hpp>

struct enum_names;

namespace output {
  //! \ingroup grp_output
  //! All kinds of logging used by everything.  I.e a presentation class.
  class logger : boost::noncopyable {
    public:
    enum modules {
      m_config,
      m_pipeline,
      m_server,
      m_player
    };

    enum types {
      e_trace,
      e_info,
      e_warn,
      e_error,
      e_fatal
    };

    private:
    logger() : file_(NULL), console_(NULL) {}

    public:
    void write(enum modules, enum types, const char *format, va_list);

    // TODO:
    //   Can't decide whether to have this static bit and have partial
    //   construction, or to have total construction in main but store a static
    //   pointer... the general convention is to have a separate configure
    //   function, but most of t he classes are valid on construct.

    static logger &global_logger() { return s_global_logger_; }

    private:
    void write_log(FILE *, const enum_names &, const char *format, va_list);
    void write_console(FILE *, const enum_names &, const char *format, va_list);

    private:
    typedef boost::mutex mutex_type;
    typedef mutex_type::scoped_lock lock_type;

    static logger s_global_logger_;

    FILE *file_;
    FILE *console_;
    mutex_type mutex_;
  };

  //! \ingroup grp_output
  //! Instance of the logger for a particular module.
  class module_logger : boost::noncopyable {
    public:
    module_logger(logger::modules);

    void error(const char *f, ...);
    void fatal(const char *f, ...);
    void warn(const char *f, ...);
    void info(const char *f, ...);
    void trace(const char *f, ...);

    private:
    enum logger::modules module_;
  };

  // TODO:
  //   Particular stages will want their own module prefixes, so this will need
  //   to deal with them.
  class stage_logger {};

  //! \ingroup grp_output
  //! Quick alias so we can undefine it sometimes.
#define NERVE_TRC(module__, ...)\
  ::output::logger::global_logger().write(module__, ::output::logger::e_trace, __VA_ARGS__);

} // ns
#endif
