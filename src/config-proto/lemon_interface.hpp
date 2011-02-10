// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * See class config::lemon_interface.
 */

#ifndef CONFIG_LEMON_PARSER_HPP_jncvazqg
#define CONFIG_LEMON_PARSER_HPP_jncvazqg

#include "flex_interface.hpp"
#include "nerve_config.hpp"

#include <boost/cstdint.hpp>

namespace config { class parse_context; }

namespace {
  // defined up here because we don't know about the class lemon_interface yet.
  typedef config::parse_context parse_context_type;
}

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void ParseFree(void *lemon_interface, void (*freeProc)(void*));
extern void Parse(void *lemon_interface, int token_id, ::config::flex_interface::token_data, parse_context_type *);

#ifndef NERVE_PARSER_TRACE
#  error "NERVE_PARSER_TRACE should be defined by nerve_config.hpp"
#elif NERVE_PARSER_TRACE
extern void ParseTrace(FILE *stream, char *zPrefix);
#endif

#include <cstdlib>

namespace config {
  namespace detail {
#ifndef NERVE_PARSER_TRACE
#  error "NERVE_PARSER_TRACE should be defined by nerve_config.hpp"
#elif NERVE_PARSER_TRACE
    static char parse_trace_prefix[] = "lemon: ";
#endif

    //! Wrapping std::allocator.
    void *parser_alloc(size_t bytes);
    void parser_free(void *mem);
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
      context_type *context() const { return context_; }

      params &trace(bool v) { trace_ = v; return *this; }
      params &context(context_type *c) { NERVE_ASSERT_PTR(c); context_ = c; return *this; }

      private:
      context_type *context_;
      bool trace_;
    };

    lemon_interface(const params &p) {
      state_ = ::ParseAlloc(&detail::parser_alloc);
      if (p.trace()) {
#if NERVE_PARSER_TRACE
        ::ParseTrace(stdout, detail::parse_trace_prefix);
#endif
      }
      context_ = p.context();
    }

    ~lemon_interface() {
      ::ParseFree(state_, &detail::parser_free);
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

    context_type *context_;
    void *state_;
  };
}

#endif
