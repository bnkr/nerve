// Copyright (c) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <execinfo.h>
#include <cstdlib>

#include "backtrace.hpp"

/*****************
 * Raw backtrace *
 *****************/

raw_backtrace::raw_backtrace() {
  int got = 0;
  // Keep increasing allocation until we can fit the entire stack in there.
  {
    int max_size = 128;
    do {
       addrs_.reset(new (void*)[max_size]);;
       got = ::backtrace(addr_, max_size);
       max_size += 128;
    } while (got == max_size);
  }

  if (got < 1) {
    size_ = 0;
  }
  else {
    size_ = got - 1;
  }
}

/********************
 * Pretty backtrace *
 ********************/

pretty_backtrace::pretty_backtrace() {
  raw_backtrace rbt;
  execinfo_init();

  // TODO:
  //   This is the most basic and least useful trace.  Using dlinfo we can get
  //   more advanced data.  Using DWARF, we should be able to get even further
  //   (although I don't know whether we might need dlinfo as well).

  strings_ = ::backtrace_symbols(rbt.addresses(), rbt.size());
  if (strings_ == NULL) {
    throw std::bad_alloc();
  }
  size_ = backtrace_size - offset_;
}

bool pretty_backtrace::empty() const { return false; }

pretty_backtrace::iterator pretty_backtrace::begin() const { return NULL;  }
pretty_backtrace::iterator pretty_backtrace::end() const { return NULL; }

