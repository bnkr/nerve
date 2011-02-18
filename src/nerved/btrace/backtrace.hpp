// Copyright (c) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef BTRACE_BACKTRACE_HPP_i4ayhxj5
#define BTRACE_BACKTRACE_HPP_i4ayhxj5

#include <boost/scoped_array.hpp>
#include <boost/utility.hpp>

#include <vector>
#include <string>
#include <cstdlib>

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
      //! Name of the elf object (or whatever)
      const char *object() const { return object_; }
      //! Where the object is mapped.
      const void *object_address() const { return object_address_; }

      //! Demangled symbol name.  Symbol is null at the position of error.
      const char *symbol() const { return symbol_.c_str(); }
      //! Location of the symbol acording to the linker.
      const void *symbol_address() const { return symbol_address_; }

      // TODO:
      //   Not sure about the locations.  Does call mean where this was called
      //   or where we left that function?

      //! Location where the symbol is defined.
      const char *symbol_file() const { return symbol_file_; }
      //! Line of definition.
      int symbol_line() const { return symbol_line_; }

      //! Location where the symbol was called
      const char *call_file() const { return call_file_; }
      //! Line of call.
      int call_line() const { return call_line_; }

      const char *object_;
      const void *object_address_;

      std::string symbol_;
      const void *symbol_address_;
      const char *symbol_file_;
      int symbol_line_;

      const char *call_file_;
      int call_line_;

    };

    typedef std::vector<call> stack_type;
    typedef stack_type::const_iterator iterator;

    pretty_backtrace();
    ~pretty_backtrace();

    //! Some platforms can't find a backtrace.
    bool empty() const { return stack().empty(); }

    //! Call stack with the most recent call first.
    iterator begin() const { return stack_.begin(); }
    iterator end() const { return stack().end(); }

    const stack_type &stack() const { return stack_; }
    stack_type &stack() { return stack_; }

    private:
    void clean_stack(call &);
    stack_type stack_;
  };
}

#endif
