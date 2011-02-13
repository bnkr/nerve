#include "parse.hpp"

#include "../config_defines.hpp"

#include <cstring>
#include <cstdio>

using cli::settings;

namespace cli {
  //! Holds all the parsing state and modifies the settings.
  struct cli_parser {
    explicit cli_parser(settings &s) : s_(s), status_(parse_ok) {}

    void parse(int argc, char **argv);

    bool strequal(const char *a, const char *b) { return std::strcmp(a, b) == 0; }

    void arg_error(const char *, const char *);
    void arg_value_error(const char *, const char *, const char *);
    void help();
    void version();

    cli::parse_status status() const { return status_; }


    private:
    settings &s_;
    cli::parse_status status_;
  };
}

using cli::cli_parser;

#define BOOLEAN_OPT(name__, assign__)\
  else if (strequal(a, name__)) {\
    s_.assign__ = true;\
  }\

#define VALUE_OPT(name__, assign__)\
  else if (strequal(a, name__)) {\
    s_.assign__ = a;\
  }

#define APPEND_OPT(name__, assign__)\
  else if (strequal(a, name__)) {\
    s_.assign__.push_back(a);\
  }

void cli_parser::parse(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    const char *const a = argv[i];
    if (strequal(a, "-help")) {
      help();
      return;
    }
    else if (strequal(a, "-version")) {
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
      arg_error(a, "unrecognised argument");
    }
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
    "Nerve daemon, version " NERVED_VERSION
    "Copyright (C) James Webber 2008-2011\n"
    "Under a 3-clause BSD license.\n"
  );

  status_ = cli::parse_exit;
}

void cli_parser::arg_error(const char *arg, const char *what) {
  std::fprintf(stderr, "nerve: %s: %s\n", arg, what);
  status_ = cli::parse_fail;
}

void cli_parser::arg_value_error(const char *arg, const char *val, const char *what) {
  std::fprintf(stderr, "nerve: %s '%s': %s\n", arg, val, what);
  status_ = cli::parse_fail;
}

cli::parse_status cli::parse(settings &s, int argc, char **argv) {
  cli_parser parser(s);
  parser.parse(argc, argv);
  return parser.status();
}
