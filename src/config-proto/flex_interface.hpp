// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Interface to the flex based lexer.
 */

#ifndef CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q
#define CONFIG_FLEX_INTERFACE_HPP_8ovjfk6q

#include "../plugin-proto/asserts.hpp"

#include <cstdio>
#include <algorithm>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

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
    //! Initialiser data.
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

    //! Clean up memory.
    void destroy();

    //! Text of the last matched token.  Note: this won't work to get the token
    //! text of where the parse actually is, because the lookahead token
    //! overwrites it.
    inline token_type get_current_token() { return detail::current_token; }

    //! Get the next token.
    inline int next_token() { return ::yylex(); }

    //! Free text which was in token_data.text
    extern void free_text(char *data);

    //@}

    // TODO:
    //   These next bits should be replaced with the unique_ptr thing.

    //! This is basically required because of no move construcrtors.
    typedef boost::shared_ptr<char> text_ptr;

    //! These are indended only for use by the parser.
    inline text_ptr make_text_ptr(char *text) { return text_ptr(NERVE_CHECK_PTR(text), &free_text); }
    inline text_ptr make_text_ptr(token_type tok) { return make_text_ptr(NERVE_CHECK_PTR(tok.text)); }

    //! \ingroup grp_config_lexer
    //! Always initialise with the correct deleter!
    // TODO:
    //   This should be made impossibble to construct without an existing
    //   unique_ptr.
    typedef boost::shared_ptr<char> shared_ptr;

    //! \ingroup grp_config_lexer
    //! Exclusive ownership pointer.  It's passed by reference and can be taken
    //! over if wanted but still does a checked delete.
    class unique_ptr : boost::noncopyable {
      public:

      //! Used to return a pointer by value directly into an exclusive
      //! ownership pointer
      struct mover {
        friend class unique_ptr;

        explicit mover(char *data) : d_(data) {}

        private:
        char *take() { return d_; }
        char *d_;
      };

      typedef void(*deleter_type)(char*);
      typedef flex_interface::shared_ptr shared_type;

      unique_ptr() : p_(NULL) {}
      unique_ptr(mover p) : p_(p.take()) {}
      explicit unique_ptr(char *p) : p_(p) {}

      ~unique_ptr() {
        if (p_) {
          free_text(p_);
        }
      }

      deleter_type deleter() const { return &free_text; }
      char *get() const { return p_; }

      shared_type release_shared() {
        NERVE_ASSERT(p_ != NULL, "ownership must not have been taken yet");
        shared_type tmp(p_, deleter());
        p_ = NULL;
        return tmp;
      }

      mover release_exclusive() {
        NERVE_ASSERT(p_ != NULL, "ownership must not have been taken yet");
        unique_ptr::mover p(p_);
        p_ = NULL;
        return p;
      }

      char *get() { return p_; }

      private:
      char *p_;
    };
  }
}

#endif
