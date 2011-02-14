// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "logging.hpp"

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

  const char *module_to_name(modules_type m) {
    switch (m) {
    case modules::config:
      return "config";
    case modules::player:
      return "player";
    case modules::server:
      return "server";
    case modules::pipeline:
      return "server";
    }
  }

  const char *type_to_name(messages_type t) {
    switch (t) {
    case messages::error:
      return "error";
    case messages::warn:
      return "warn";
    case messages::info:
      return "info";
    case messages::trace:
      return "trace";
    case messages::fatal:
      return "fatal";
    }
  }

  enum_names enums_to_name(modules_type m, messages_type t) {
    enum_names ns;
    ns.module = module_to_name(m);
    ns.type = type_to_name(t);
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

void message::write_prefix(modules_type m, messages_type t) {
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
  const enum_names names = enums_to_name(m, t);

  detail::log_data &ld = detail::get_data();

  // Doing it this way means the above computation doesn't need to be locked.
  ld.mutex().lock();

  ld.printf(
    "%s %d %d:%d:%d: %s: %s",
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

logger::logger(modules_type m)
: module_(m) {
}

void logger::write(messages_type t, const char *f, ...) {
  if (! message::should_write(*this, t)) return;
  message ms(*this, t);
  va_list args;
  va_start(args, f);
  ms.vprintf(f, args);
  va_end(args);
}

#define LOGGER_FUNC(category__)\
  void logger::category__(const char *format, ...) {\
    if (! message::should_write(*this, messages::category__)) return;\
    message ms(*this, messages::category__);\
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
