// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <cstdarg>
#include <cstdio>

namespace config {
  /*!
   * \ingroup grp_config
   *
   * This has been abstracted because probably we'll want to duplicate errors
   * to a log file as well as stdout.  Some kind of special handling anwyay, so
   * it means we can't just dump it on stdout.
   */
  class error_reporter {
    public:

    // TODO:
    //   Ideally the format would be derrived from a big list of error
    //   constants.  Then we can document everything together.

    error_reporter() : error_(false), fatal_error_(false) {}

    void report() { error_ = true;  }
    void report_fatal() { fatal_error_ = true; }

    void report(const char *format, ...) {
      error_ = true;
      va_list args;
      va_start(args, format);
      std::vfprintf(stderr, format, args);
      std::fprintf(stderr, "\n");
      va_end(args);
    }

    void report_fatal(const char *format, ...) {
      fatal_error_ = true;
      va_list args;
      va_start(args, format);
      std::vfprintf(stderr, format, args);
      std::fprintf(stderr, "\n");
      va_end(args);
    }

    bool error() const { return error_ || fatal_error_; }
    bool fatal_error() const { return fatal_error_; }

    private:

    void print_file_line() {
      std::fprintf(stderr, "<file>:<line>: ");
    }

    bool error_;
    bool fatal_error_;
  };
}
