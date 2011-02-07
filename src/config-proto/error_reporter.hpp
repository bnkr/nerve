// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <cstdarg>
#include <cstdio>
#include <boost/utility.hpp>

namespace config {

  /*!
   * \ingroup grp_config
   * Locational state handled by the lexer (and whatever bit knows about he
   * filename).
   */
  class parse_location {
    public:

    parse_location() : file_(NULL), line_(1) {}

    int line() const { return line_; }
    const char *file() const { return file_; }

    void increment() { ++line_; }
    void new_file(const char *file) { file_ = file; }

    private:
    const char *file_;
    int line_;
  };
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
      // so it syncs properly with the trace output
      stream_ = stdout;
    }

    void report() { error_ = true;  }
    void report_fatal() { fatal_error_ = true; }

    void report(const char *format, ...) {
      error_ = true;
      va_list args;
      va_start(args, format);
      print_file_line();
      std::vfprintf(stream_, format, args);
      std::fprintf(stream_, "\n");
      va_end(args);
    }

    void report_fatal(const char *format, ...) {
      fatal_error_ = true;
      va_list args;
      va_start(args, format);
      print_file_line();
      std::vfprintf(stream_, format, args);
      std::fprintf(stream_, "\n");
      va_end(args);
    }

    bool error() const { return error_ || fatal_error_; }
    bool fatal_error() const { return fatal_error_; }

    void increment_line() { location_.increment(); }

    const parse_location &location() const { return location_; }
    parse_location &location() { return location_; }

    private:

    void print_file_line() const {
      std::fprintf(stream_, "%s:%d: ", this->location().file(), this->location().line());
    }

    parse_location location_;
    bool error_;
    bool fatal_error_;
    FILE *stream_;
  };
}
