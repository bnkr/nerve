// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "logger.hpp"

#include <cstdio>
#include <boost/date_time.hpp>

using output::module_logger;
using output::logger;

struct enum_names {
  const char *module;
  const char *type;
};

namespace {
  const char *module_to_name(logger::modules m) {
    switch (m) {
    case logger::m_config:
      return "config";
    case logger::m_player:
      return "player";
    case logger::m_server:
      return "server";
    case logger::m_pipeline:
      return "server";
    }
  }

  const char *type_to_name(logger::types t) {
    switch (t) {
    case logger::e_error:
      return "error";
    case logger::e_warn:
      return "warn";
    case logger::e_info:
      return "info";
    case logger::e_trace:
      return "trace";
    case logger::e_fatal:
      return "fatal";
    }
  }

  enum_names enums_to_name(logger::modules m, logger::types t) {
    enum_names ns;
    ns.module = module_to_name(m);
    ns.type = type_to_name(t);
    return ns;
  }
}

void logger::write(modules m, types t, const char *f, va_list args) {
  const enum_names ns = enums_to_name(m, t);
  if (file_) write_log(file_, ns, f, args);
  if (console_) write_console(console_, ns, f, args);
}

void logger::write_console(FILE *s, const enum_names &ns, const char *format, va_list args) {
  // TODO:
  //   Use a different format.
  write_log(s, ns, format, args);
}

void logger::write_log(FILE *s, const enum_names &ns, const char *format, va_list args) {
  namespace pt = boost::posix_time;
  namespace gt = boost::gregorian;
  const pt::ptime now = pt::second_clock::local_time();
  const gt::date date = now.date();
  const pt::time_duration time = now.time_of_day();

  unsigned short day = date.day();
  const char *month = date.month().as_short_string();
  lock_type lk(mutex_);
  std::fprintf(s, "%s %d %d:%d:%d: ", month, day, time.hours(), time.minutes(), time.seconds());
  std::fprintf(s, "%s: %s: ", ns.module, ns.type);
  std::vfprintf(s, format, args);
}

module_logger::module_logger(logger::modules m)
: module_(m) {
}

#define LOGGER_FUNC(category__)\
  void module_logger::category__(const char *format, ...) {\
    va_list args;\
    va_start(args, format);\
    logger::global_logger().write(this->module_, logger::e_ ## category__, format, args);\
    va_end(args);\
  }

LOGGER_FUNC(fatal)
LOGGER_FUNC(error)
LOGGER_FUNC(warn)
LOGGER_FUNC(info)
LOGGER_FUNC(trace)
