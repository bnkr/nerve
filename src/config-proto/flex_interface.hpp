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

    //! Text of the last matched token.  Note: this won't work to get the token
    //! text of where the parse actually is, because the lookahead token
    //! overwrites it.
    inline token_type get_current_token() { return detail::current_token; }

    //! Get the next token.
    inline int next_token() { return ::yylex(); }

    //! Free text which was in token_data.text
    extern void free_text(char *data);

    //! For passing a lexical analysed pointer about.
    class pass_text {
      public:

      friend class text_ptr;

      explicit pass_text(char *data) { p_ = NERVE_CHECK_PTR(data); }
      pass_text(const pass_text &p) { p_ = p.take(); }

      ~pass_text() {
        NERVE_ASSERT(this->p_ == NULL, "pass_text is being deleted without the pointer being taken");
      }

      private:

      char *take() const {
        NERVE_ASSERT(this->p_ != NULL, "ownership has already been taken");
        char *ret = NULL;
        std::swap(ret, this->p_);
        return ret;
      }

      mutable char *p_;
    };

    //! Raii pointer for string token data.
    class text_ptr {
      public:
      text_ptr() : p_(NULL) {}

      // We have to do all this hax with const because otherwise you can't pass
      // temporaries around properly.
      text_ptr(const pass_text &pt) : p_(pt.take()) { }
      text_ptr &operator=(const pass_text &pt) { p_ = pt.take(); return *this; }

      text_ptr(const text_ptr &tp) {
        // This is necessary because we need to store these as object members
        // and those objects need to be copied.  We don't have the move
        // constructor so we just have to make do here.
        NERVE_ASSERT(tp.get() == NULL, "an initialised text_ptr must not be copied");
      }

      ~text_ptr() { if (p_) { flex_interface::free_text(p_); } }

      const char *use() const { return NERVE_CHECK_PTR(p_); }
      const char *get() const { return p_; }

      private:
      char *p_;
    };

    //@}
  }
}

#endif
