// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Interface to the flex based lexer.
 */

#ifndef CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q
#define CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q

#include <cstdio>
#include "../plugin-proto/asserts.hpp"

extern int yylex();

namespace config {
  class parse_context;

  namespace flex_interface {
    //! The semantic value of a token: some text, Text/value stuff.
    union token_data {
      //! Arbitrary textual data (e.g a string literal or identifier)
      char   *text;
    };

    // Only declared here due to inline accessors.
    namespace detail { extern token_data current_token; }

    //! Used for repoting errors etc.
    typedef ::config::parse_context context_type;
    //! The "meaning" of the token.
    typedef token_data token_type;

    //! \ingroup grp_config_lexer
    struct params {
      params() : stream_(NULL), trace_(false), context_(NULL) {}

      FILE *stream() const { return stream_; }
      bool trace() const { return trace_; }
      context_type *context() const { return context_; }

      params &stream(FILE *f) { NERVE_ASSERT_PTR(f); stream_ = f; return *this; }
      params &trace(bool v) { trace_ = v; return *this; }
      params &context(context_type *c) { NERVE_ASSERT_PTR(c); context_ = c; return *this; }

      private:
      FILE *stream_;
      bool trace_;
      context_type *context_;
    };

    //@{
    //! \ingroup grp_config_lexer

    //! The flex_interface is static so it gets a free function.
    void init(const params &);

    //! Text of the last matched token.  Note: this won't work to get the token
    //! text of where the parse actually is, because the lookahead token
    //! overwrites it.
    inline token_data get_current_token() { return detail::current_token; }

    //! Get the next token.
    inline int next_token() { return ::yylex(); }

    //! Free text which was in token_data.text
    extern void free_text(char *data);
    //@}

  }
}

#endif
