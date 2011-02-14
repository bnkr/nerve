// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "settings.hpp"

using cli::settings;

settings::settings() {
  trace_parser_ = trace_lexer_ = false;
  dump_config_ = false;
  state_ = NULL;
  socket_ = NULL;
  log_ = NULL;
}
