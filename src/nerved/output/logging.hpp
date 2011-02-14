// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef OUTPUT_LOGGING_HPP_1uj7g16h
#define OUTPUT_LOGGING_HPP_1uj7g16h

#include <cstdarg>
#include <cstdio>

#include <boost/thread.hpp>
#include <boost/utility.hpp>

namespace output {
  //! \ingroup grp_output
  //! List of message sources.
  namespace modules {
    enum modules_e {
      config,
      pipeline,
      server,
      player
    };
  }

  //! \ingroup grp_output
  //! List of message severities.
  namespace messages {
    enum messages_e {
      trace,
      info,
      warn,
      error,
      fatal
    };
  }

  typedef modules::modules_e   modules_type;
  typedef messages::messages_e messages_type;

  namespace detail {
    //! Used by other logging bits.  This class does no checking of whether a
    //! log should be written and it does not lock the streams.
    class log_data : boost::noncopyable {
      public:
      typedef boost::mutex mutex_type;
      typedef mutex_type::scoped_lock lock_type;

      static log_data &instance() { return s_instance_; }

      bool should_write(modules_type m, messages_type t) {
        // TODO:
        //   Other properties apply here.
        return console_ || log_;
      }

      //! Write the actual data.
      void printf(const char *format, ...) {
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
    explicit logger(modules_type);

    bool should_write(messages_type t) {
      return detail::get_data().should_write(module_, t);
    }

    void write(messages_type, const char *f, ...);

    void error(const char *f, ...);
    void fatal(const char *f, ...);
    void warn(const char *f, ...);
    void info(const char *f, ...);
    void trace(const char *f, ...);

    modules_type module() const { return module_; }

    private:
    modules_type module_;
  };

  //! \ingroup grp_output
  //! Scope-locked logging context for one message.  This is used when it's
  //! necessary to mix printf and vprintf in the same call.  This does not
  //! automatically check whether a message should be written.
  class message : boost::noncopyable {
    public:
    inline static bool should_write(modules_type m, messages_type t) {
      return detail::get_data().should_write(m, t);
    }

    inline static bool should_write(logger &l, messages_type t) {
      return detail::get_data().should_write(l.module(), t);
    }

    message(modules_type m, messages_type t)
    { write_prefix(m, t); }

    message(logger &l, messages_type t)
    { write_prefix(l.module(), t); }

    ~message() { detail::get_data().mutex().unlock(); }

    void printf(const char *f, ...);
    void vprintf(const char *f, va_list);

    private:
    void write_prefix(modules_type m, messages_type t);
  };

  // TODO:
  //   Particular stages will want their own module prefixes, so this will need
  //   to deal with them.
  class stage_message {};
} // ns
#endif
