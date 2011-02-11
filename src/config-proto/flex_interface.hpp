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

    //! \ingroup grp_config_lexer
    //! Always initialise with the correct deleter!
    // TODO:
    //   This should be made impossibble to construct without an existing
    //   unique_ptr.
    typedef boost::shared_ptr<char> shared_ptr;

    //! \ingroup grp_config_lexer
    //! Exclusive ownership pointer.
    class unique_ptr : boost::noncopyable {
      public:

      //! Used to return a pointer by value directly into an exclusive
      //! ownership pointer
      struct mover {
        friend class unique_ptr;
        explicit mover(char *data) : d_(NERVE_CHECK_PTR(data)) {}

        private:

        char *take() const {
          char *tmp = d_;
          d_ = NULL;
          return tmp;
        }

        mutable char * d_;
      };

      unique_ptr() : p_(NULL) {}
      unique_ptr(mover p) : p_(p.take()) {}
      explicit unique_ptr(char *p) : p_(p) {}

      unique_ptr &operator=(mover p) {
        NERVE_ASSERT(p_ == NULL, "unique_ptr must not be initialised twice");
        p_ = p.take();
        return *this;
      }

      ~unique_ptr() {
        if (p_) {
          free_text(p_);
        }
      }

      char *get() const { return p_; }


      private:
      char *p_;
    };

    /*!
     * \ingroup grp_config_lexer
     *
     * Used only for transfering ownership to something else.  If the pointer is
     * not released, it is deleted anyway.  This object must be passed by
     * reference, hence why it's not called a transfer_ptr.
     *
     * This stuff doesn't go in the unique_ptr because otherwise it's ambigous
     * whether it's intended to transfer ownership or not and the holder of the
     * unique pointer could have its pointer wiped unexpectedly.
     */
    class transfer_mem : boost::noncopyable {
      public:
      typedef void(*deleter_type)(char*);
      typedef flex_interface::shared_ptr shared_type;
      typedef flex_interface::unique_ptr unique_type;

      explicit transfer_mem(char *p) : p_(NERVE_CHECK_PTR(p)) {}
      ~transfer_mem() {
        if (p_) {
          deleter()(p_);
        }
      }

      shared_type release_shared() {
        NERVE_ASSERT(p_ != NULL, "ownership must not have been taken yet");
        shared_type tmp(p_, deleter());
        p_ = NULL;
        return tmp;
      }

      unique_ptr::mover release_exclusive() {
        NERVE_ASSERT(p_ != NULL, "ownership must not have been taken yet");
        unique_ptr::mover p(p_);
        p_ = NULL;
        return p;
      }

      deleter_type deleter() const { return &free_text; }

      private:
      char *p_;
    };
  }
}

#endif
