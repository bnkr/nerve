#include "settings.hpp"
#include "tune_config.hpp"

namespace {
  const char *usage_message() {
    return
      "usage: tune [-l] [-t|-m S] [abcdefg[#|B]]...\n"
      "Play one or more notes in order.  Notes are a-g with a # or B suffix.  Options\n"
      "and arguments can be in any order.   With no notes, it defaults to c.\n"
      "\n"
      "Options:\n"
      "  -h        This message and quit.\n"
      "  -q        Don't print anything except error messages.\n"
      "  -l        Loop playing notes.\n"
      "  -t S      Number of seconds to play note for.  Defaults to 1.\n"
      "  -m M      Number of miliseconds to play note for.  Defaults to 100.\n"
      "\n"
      "Copyright (C) 2009, James Webber.\n"
      "Version " PROJECT_VERSION ".  Under a 3 clause BSD license.\n"
      ;
  }
}


