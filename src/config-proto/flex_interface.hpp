// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Interface to the flex based lexer.
 */

#ifndef CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q
#define CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q

#include <cstdio>

extern int yylex();

namespace config {
  namespace flex_interface {
    //! The semantic value of a token: some text, Text/value stuff.
    union token_data {
      //! Arbitrary textual data (e.g a string literal or identifier)
      char   *text;
    };

    namespace detail {
      extern token_data current_token;
    }

    //! \ingroup grp_config_lexer
    struct params {
      params() : stream_(NULL), trace_(false) {}

      FILE *stream() const { return stream_; }
      bool trace() const { return trace_; }

      params &stream(FILE *f) { stream_ = f; return *this; }
      params &trace(bool v) { trace_ = v; return *this; }

      private:
      FILE *stream_;
      bool trace_;
    };

    //@{
    //! \ingroup grp_config_lexer

    //! The flex_interface is static so it gets a free function.
    void init(const params &);

    //! Error at end of document.
    bool error();
    //! Stop immediately.
    bool fatal_error();

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
