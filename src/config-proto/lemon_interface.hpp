// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * See class config::lemon_interface.
 */

#ifndef CONFIG_LEMON_PARSER_HPP_jncvazqg
#define CONFIG_LEMON_PARSER_HPP_jncvazqg

#include "flex_interface.hpp"

namespace {
  // defined up here because we don't know about the class lemon_interface yet.
  typedef void * parse_context_type;
}

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void ParseFree(void *lemon_interface, void (*freeProc)(void*));
extern void Parse(void *lemon_interface, int token_id, ::config::flex_interface::token_data, parse_context_type);

#if defined(ENABLE_PARSER_TRACE) && ENABLE_PARSER_TRACE
extern void ParseTrace(FILE *stream, char *zPrefix);
#endif

#include <cstdlib>

namespace config {
  namespace detail {
#if defined(NERVE_ENABLE_PARSER_TRACE) && NERVE_ENABLE_PARSER_TRACE
    static char parse_trace_prefix[] = "config lemon_interface:";
#endif
  }

  //! \ingroup grp_config_parser
  //! Simple C++ish interface to the lemon_interface.
  class lemon_interface {
    public:

    //! Type used to supply info to the lemon_interface.
    typedef parse_context_type context_type;

    //! Initialiser.
    struct params {
      params() : context_(NULL), trace_(false) {}

      bool trace() const { return trace_; }
      context_type context() const { return context_; }

      params &trace(bool v) { trace_ = v; return *this; }
      params &context(context_type c) { context_ = c; return *this; }

      private:
      context_type context_;
      bool trace_;
    };

    lemon_interface(const params &p) {
      state_ = ::ParseAlloc(malloc);
      if (p.trace()) {
#if defined(NERVE_ENABLE_PARSER_TRACE) && NERVE_ENABLE_PARSER_TRACE
        ::ParseTrace(stdout, parse_trace_prefix);
#endif
      }
    }

    ~lemon_interface() {
      ::ParseFree(state_, free);
    }

    //! Call for each token.  Some other staate mustbe used to work out if there
    //! was an error.
    void token(int tok) {
      ::Parse(state_, tok, flex_interface::get_current_token(), context_);
    }

    //! Must be called after everything else, but not point in calling if
    //! there was an error.
    void finish() {
      flex_interface::token_data tok_val;
      ::Parse(state_, 0, tok_val, context_);
    }

    private:

    context_type context_;
    void *state_;
  };
}

#endif
