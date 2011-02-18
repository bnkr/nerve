// Copyright (c) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef BTRACE_BACKTRACE_HPP_i4ayhxj5
#define BTRACE_BACKTRACE_HPP_i4ayhxj5

#include <boost/scoped_array.hpp>

namespace btrace {
  /*!
   * \ingroup grp_backtrace
   *
   * Uses various hax and horrible imporable methods to retrive a list of the
   * instruction addresses on the stack.  The most recent call is *last* in the
   * list and will be the calling function (not any function of raw_backtrace).
   */
  class raw_backtrace {
    public:
    typedef void const *const *addresses_type;

    raw_backtrace();

    typedef addresses_type iterator;

    iterator begin() const { return addresses(); }
    iterator end() const { return (addresses() + this->size()); }

    addresses_type addresses() const { return addrs_.get(); }
    size_t size() const { return size_; }

    private:
    size_t size_;
    boost::scoped_array<void*> addrs_;
  };

  /*!
   * \ingroup grp_backtrace
   * Used to store demangled/resolved backtrace from raw_backtrace.
   */
  class pretty_backtrace : boost::noncopyable {
    public:

    struct call {
      // TODO:
      //   This will prolly change to accomodate whatever data we can extract
      //   from the data.

      //! Demangled symbol name.  Symbol is null at the position of error.
      const char *symbol() const { return symbol_; }
      //! File containing symbol
      const char *file() const { return file_; }
      //! Line of call.
      int line() const { return line_; }

      const char *file_;
      int line_;
      const char *symbol_;
    };

    typedef call const * iterator;

    pretty_backtrace();

    //! Some platforms can't find a backtrace.
    bool empty() const;

    //! Call stack with the most recent call first.
    iterator begin() const;
    iterator end() const;
  };
}

#endif
