// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "error_reporter.hpp"

namespace config {
  class pipeline_config;

  //! \ingroup grp_config
  //! Parsing context used by the lexer and parser.
  class parse_context {
    public:
    parse_context(pipeline_config &pc) : output_(pc) {}

    error_reporter &reporter() { return reporter_; }
    pipeline_config &output() { return output_; }

    private:
    pipeline_config &output_;
    error_reporter reporter_;
  };
}