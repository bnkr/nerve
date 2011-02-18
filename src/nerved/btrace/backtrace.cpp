// Copyright (c) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "backtrace.hpp"
#include "demangle.hpp"

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <boost/bind.hpp>
#include <execinfo.h>
#include <dlfcn.h>

using namespace btrace;

/*****************
 * Raw backtrace *
 *****************/

raw_backtrace::raw_backtrace() {
  int got = 0;
  // Keep increasing allocation until we can fit the entire stack in there.
  {
    int max_size = 128;
    do {
       addrs_.reset(new void*[max_size]);;
       got = ::backtrace(addrs_.get(), max_size);
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

static const char unknown_file[] = "file unknown";
static const char unknown_function[] = "file unknown";

pretty_backtrace::pretty_backtrace() {
  raw_backtrace rbt;

  stack_.assign(rbt.size(), pretty_backtrace::call());

  size_t index = 0;
  raw_backtrace::iterator e = rbt.end();
  for (raw_backtrace::iterator i = rbt.begin(); i != e; ++i) {
    // this points to somewhere in the function that will be returned to
    const void *const return_pointer = *i;

    call &current = stack_[index++];

    Dl_info info;
    int ret = ::dladdr(return_pointer, &info);
    if (ret == 0) {
      continue;
    }

    current.object_address_ = info.dli_fbase;

    if (info.dli_fname) {
      current.object_ = info.dli_fname;
    }

    if (info.dli_saddr) {
      current.symbol_address_ = info.dli_saddr;
    }

    if (info.dli_sname) {
      current.symbol_ = btrace::demangle_name(info.dli_sname);
    }

    // TODO:
    //   Now use dwarf to find the location of return_pointer.
  }
}

pretty_backtrace::~pretty_backtrace() {
  std::for_each(stack_.begin(), stack_.end(), boost::bind(&pretty_backtrace::clean_stack, this, _1));
}

void pretty_backtrace::clean_stack(call &c) {
}
