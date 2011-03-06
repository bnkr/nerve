// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "logging.hpp"

#include "../util/asserts.hpp"

#include <cstdio>
#include <boost/date_time.hpp>

using namespace output;

/***********
 * Utility *
 ***********/

namespace {
  struct enum_names {
    const char *module;
    const char *type;
  };

  const char *source_to_name(source_type s) {
    switch (s) {
    case source::config:
      return "config";
    case source::player:
      return "player";
    case source::server:
      return "server";
    case source::pipeline:
      return "pipeline";
    }

    NERVE_ABORT("impossible value for message source");
    return "(invalid)";
  }

  const char *type_to_name(cat_type c) {
    switch (c) {
    case cat::error:
      return "error";
    case cat::warn:
      return "warn";
    case cat::info:
      return "info";
    case cat::trace:
      return "trace";
    case cat::fatal:
      return "fatal";
    }

    NERVE_ABORT("impossible value for message type");
    return "(invalid)";
  }

  enum_names enums_to_name(source_type s, cat_type c) {
    enum_names ns;
    ns.module = source_to_name(s);
    ns.type = type_to_name(c);
    return ns;
  }

  // Saves us linking the whole of boost.time_date
  const char *month_to_string(boost::gregorian::greg_month m) {
    switch (m.as_number()) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 6: return "May";
    case 7: return "Jun";
    case 8: return "Jul";
    case 9: return "Aug";
    case 10: return "Sep";
    case 11: return "Oct";
    case 12: return "Dec";
    default: return "<argh>";
    }
  }
}

/********************
 * Singleton Logger *
 ********************/

detail::log_data detail::log_data::s_instance_;

/*******************
 * Message Context *
 *******************/

void message::write_prefix(source_type s, cat_type c) {
  namespace pt = boost::posix_time;
  namespace gt = boost::gregorian;
  const pt::ptime now = pt::second_clock::local_time();
  const gt::date date = now.date();
  const pt::time_duration time = now.time_of_day();

  const unsigned short day = date.day();
  const char *const month = month_to_string(date.month());
  const unsigned short hours = time.hours();
  const unsigned short minutes = time.minutes();
  const unsigned short seconds = time.seconds();
  const enum_names names = enums_to_name(s, c);

  detail::log_data &ld = detail::get_data();

  // Doing it this way means the above computation doesn't need to be locked.
  ld.mutex().lock();

  ld.printf(
    "%s %d %d:%d:%d [%s.%s] ",
    month, day, hours, minutes, seconds,
    names.module, names.type
  );
}

void message::printf(const char *f, ...) {
  va_list args;
  va_start(args, f);
  detail::get_data().vprintf(f, args);
  va_end(args);
}

void message::vprintf(const char *f, va_list args) {
  detail::get_data().vprintf(f, args);
}

/*********************
 * Per-Module Logger *
 *********************/

logger::logger(source_type s)
: source_(s) {
}

void logger::write(cat_type c, const char *f, ...) {
  if (! message::should_write(*this, c)) return;
  message ms(*this, c);
  va_list args;
  va_start(args, f);
  ms.vprintf(f, args);
  va_end(args);
}

#define LOGGER_FUNC(category__)\
  void logger::category__(const char *format, ...) {\
    if (! message::should_write(*this, cat::category__)) return;\
    message ms(*this, cat::category__);\
    va_list args;\
    va_start(args, format);\
    ms.vprintf(format, args);\
    va_end(args);\
  }

LOGGER_FUNC(fatal)
LOGGER_FUNC(error)
LOGGER_FUNC(warn)
LOGGER_FUNC(info)
LOGGER_FUNC(trace)
