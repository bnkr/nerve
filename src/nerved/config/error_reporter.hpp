// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef CONFIG_ERROR_REPORTER_HPP_tkskcau9
#define CONFIG_ERROR_REPORTER_HPP_tkskcau9

#include "parse_location.hpp"
#include "../output/logging.hpp"

#include <cstdarg>
#include <cstdio>
#include <boost/utility.hpp>

namespace config {
  /*!
   * \ingroup grp_config
   *
   * This has been abstracted because probably we'll want to duplicate errors
   * to a log file as well as stdout.  Some kind of special handling anwyay, so
   * it means we can't just dump it on stdout.
   */
  class error_reporter : boost::noncopyable {
    public:

    // TODO:
    //   Ideally the format would be derrived from a big list of error
    //   constants.  Then we can document everything together.

    explicit error_reporter() : error_(false), fatal_error_(false) {
    }

    void report() { error_ = true;  }
    void report_fatal() { fatal_error_ = true; }

#define NERVE_ERROR_REPORTER_REPORT(member__, loc__)\
      member__ = true;\
      va_list args;\
      va_start(args, format);\
      write_report(loc__, format, args);\
      va_end(args);

    void lreport(const parse_location &l, const char *format, ...) {
      NERVE_ERROR_REPORTER_REPORT(error_, l);
    }

    void report(const char *format, ...) {
      NERVE_ERROR_REPORTER_REPORT(error_, this->location());
    }

    void report_fatal(const char *format, ...) {
      NERVE_ERROR_REPORTER_REPORT(fatal_error_, this->location());
    }

    bool error() const { return error_ || fatal_error_; }
    bool fatal_error() const { return fatal_error_; }

    void increment_line() { location_.increment(); }

    const parse_location &location() const { return location_; }
    parse_location &location() { return location_; }

    private:

    void write_report(const parse_location &loc, const char *format, va_list args) {
      output::message m(output::source::config, output::cat::error);
      m.printf("%s:%d: ", loc.file(), loc.line());
      m.vprintf(format, args);
      m.printf("\n");
    }

    parse_location location_;
    bool error_;
    bool fatal_error_;
  };
}
#endif
