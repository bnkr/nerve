// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef OUTPUT_LOGGING_HPP_1uj7g16h
#define OUTPUT_LOGGING_HPP_1uj7g16h

#include <cstdarg>
#include <cstdio>

#include <boost/thread.hpp>
#include <boost/utility.hpp>

#ifdef __GNUC__
// Starts at 2 because +this+ is an implict argument.
#define ATTR_PRINTF   __attribute__ ((format (printf, 2, 3)))
#define ATTR_PRINTF_1 __attribute__ ((format (printf, 3, 4)))
#else
#define ATTR_PRINTF
#define ATTR_PRINTF_1
#endif

namespace output {
  //! \ingroup grp_output
  //! List of message sources.
  namespace source {
    enum source_e {
      config,
      pipeline,
      server,
      player
    };
  }

  //! \ingroup grp_output
  //! List of message severities.
  namespace cat {
    enum cat_e {
      fatal,
      error,
      warn,
      info,
      trace
    };
  }

  typedef source::source_e source_type;
  typedef cat::cat_e       cat_type;

  namespace detail {
    //! Used by other logging bits.  This class does no checking of whether a
    //! log should be written and it does not lock the streams.
    class log_data : boost::noncopyable {
      public:
      typedef boost::mutex mutex_type;
      typedef mutex_type::scoped_lock lock_type;

      static log_data &instance() { return s_instance_; }

      cat_type severity_limit() { return cat::trace; }

      bool any_outputs() const { return console_ || log_; }
      bool category_enabled(cat_type c) {
        return (int) c <= (int) severity_limit();
      }

      bool should_write(source_type, cat_type c) {
        return any_outputs() && category_enabled(c);
      }

      //! Write the actual data.
      void printf(const char *format, ...) ATTR_PRINTF {
        va_list args;
        va_start(args, format);
        this->vprintf(format, args);
        va_end(args);
      }

      void vprintf(const char *format, va_list args) {
        if (console_) std::vfprintf(console_, format, args);
        if (log_) std::vfprintf(log_, format, args);
      }

      void console(FILE *f) { console_ = f; }
      void log(FILE *f) { log_ = f; }

      mutex_type &mutex() { return mutex_; }

      private:
      log_data() : log_(NULL), console_(NULL) {}

      static log_data s_instance_;

      FILE *log_;
      FILE *console_;
      mutex_type mutex_;
    };

    inline log_data &get_data() { return log_data::instance(); }
  }

  //! \ingroup grp_output
  //! Instance of the logger for a particular module.
  class logger : boost::noncopyable {
    public:
    explicit logger(source_type);

    bool should_write(cat_type c) const {
      return detail::get_data().should_write(source_, c);
    }

    void write(cat_type, const char *f, ...) ATTR_PRINTF_1;

    void error(const char *f, ...) ATTR_PRINTF;
    void fatal(const char *f, ...) ATTR_PRINTF;
    void warn(const char *f, ...) ATTR_PRINTF;
    void info(const char *f, ...) ATTR_PRINTF;
    void trace(const char *f, ...) ATTR_PRINTF;

    source_type source() const { return source_; }

    private:
    source_type source_;
  };

  //! \ingroup grp_output
  //! Scope-locked logging context for one message.  This is used when it's
  //! necessary to mix printf and vprintf in the same call.  This does not
  //! automatically check whether a message should be written.
  class message : boost::noncopyable {
    public:
    inline static bool should_write(source_type s, cat_type c) {
      return detail::get_data().should_write(s, c);
    }

    inline static bool should_write(logger &l, cat_type c) {
      return detail::get_data().should_write(l.source(), c);
    }

    message(source_type s, cat_type c)
    { write_prefix(s, c); }

    message(logger &l, cat_type c)
    { write_prefix(l.source(), c); }

    ~message() { detail::get_data().mutex().unlock(); }

    void printf(const char *f, ...) ATTR_PRINTF;
    void vprintf(const char *f, va_list);

    private:
    void write_prefix(source_type s, cat_type c);
  };

  // TODO:
  //   Particular stages will want their own module prefixes, so this will need
  //   to deal with them.
  class stage_message {};
} // ns
#endif
