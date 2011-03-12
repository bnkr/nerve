// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "parse.hpp"

#include "../defines.hpp"

#include <cstring>
#include <cstdio>

using cli::settings;

namespace cli {
  //! Holds all the parsing state and modifies the settings.
  struct cli_parser {
    explicit cli_parser(settings &s, int argc, char **argv)
    : s_(s), i_(1), argc_(argc), argv_(argv), status_(parse_ok) {}

    void parse();

    const char *current_arg() { return argv_[i_]; }
    const char *get_value();

    bool strequal(const char *a, const char *b) { return std::strcmp(a, b) == 0; }
    bool current_equal(const char *a) { return strequal(a, current_arg()); }

    void arg_error(const char *);
    void arg_missing_error(const char *);
    void arg_value_error(const char *, const char *);
    void help();
    void version();

    cli::parse_status status() const { return status_; }

    private:
    settings &s_;
    int i_;
    int argc_;
    char **argv_;
    cli::parse_status status_;
  };
}

/*******************
 * Parse algorithm *
 *******************/

using cli::cli_parser;

#define BOOLEAN_OPT(name__, assign__)\
  else if (current_equal(name__)) {\
    s_.assign__ = true;\
  }\

#define VALUE_OPT(name__, assign__)\
  else if (current_equal(name__)) {\
    const char *const v = get_value();\
    if (v) {\
      s_.assign__ = v;\
    }\
  }

#define APPEND_OPT(name__, assign__)\
  else if (current_equal(name__)) {\
    const char *const v = get_value();\
    if (v) {\
      s_.assign__.push_back(v);\
    }\
  }

void cli_parser::parse() {
  for (i_ = 1; i_ < argc_; ++i_) {
    if (current_equal("-help")) {
      help();
      return;
    }
    else if (current_equal("-version")) {
      version();
      return;
    }
    APPEND_OPT("-cfg", config_files_)
    BOOLEAN_OPT("-cfg-dump", dump_config_)
    BOOLEAN_OPT("-cfg-trace-lexer", trace_lexer_)
    BOOLEAN_OPT("-cfg-trace-parser", trace_parser_)
    VALUE_OPT("-state", state_)
    VALUE_OPT("-socket", socket_)
    VALUE_OPT("-log", log_)
    else {
      arg_error("unrecognised argument");
    }
  }

  if (s_.config_files().empty()) {
    arg_missing_error("-cfg");
  }
}

void cli_parser::help() {
  std::printf(
    "Usage: nerved -cfg file -state file -socket file [ option ... ]\n"
    "The nerve music player daemon.\n"
    "\n"
    "Options:\n"
    "  -help      This message and exit.\n"
    "  -version   Print the version and copyright and then exit.\n"
    "\n"
    "Configuration:\n"
    "  -cfg FILE          Add a file to load as configuration.\n"
    "  -cfg-dump          Print out the config in a yaml format and exit.\n"
    "  -cfg-trace-lexer   Trace the config lexer.\n"
    "  -cfg-trace-parser  Trace the config parser (lots of output).\n"
    "\n"
    "Behavior:\n"
    "  -state FILE      Where to store and load playlist state.\n"
    "  -socket FILE     Socket to use.\n"
    "  -log FILE        File to log to or - for stderr.  Some errors are\n"
    "                   always printed on stderr.\n"
    "\n"
  );
  version();
}

void cli_parser::version() {
  std::printf(
    "Nerve daemon, version " NERVED_VERSION "\n"
    "Copyright (C) James Webber 2008-2011\n"
    "Under a 3-clause BSD license.\n"
  );

#ifndef NERVE_DEVELOPER
#  error "Need NERVE_DEVELOPER."
#elif NERVE_DEVELOPER
  std::printf("Developer build (that's why the binary's so big).\n");
#endif

  status_ = cli::parse_exit;
}

const char *cli_parser::get_value() {
  int value_i = i_ + 1;
  if (value_i >= argc_) {
    arg_error("requires a value");
    return NULL;
  }
  else {
    ++i_;
    return current_arg();
  }
}

void cli_parser::arg_error(const char *what) {
  std::fprintf(stderr, "nerve: %s: %s\n", current_arg(), what);
  status_ = cli::parse_fail;
}

void cli_parser::arg_value_error(const char *val, const char *what) {
  std::fprintf(stderr, "nerve: %s '%s': %s\n", current_arg(), val, what);
  status_ = cli::parse_fail;
}

void cli_parser::arg_missing_error(const char *which) {
  std::fprintf(stderr, "nerve: %s is required\n", which);
  status_ = cli::parse_fail;
}

/*********************
 * Parse entry point *
 *********************/

cli::parse_status cli::parse(settings &s, int argc, char **argv) {
  cli_parser parser(s, argc, argv);
  parser.parse();
  return parser.status();
}
