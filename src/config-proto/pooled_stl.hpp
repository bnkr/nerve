// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef CONFIG_POOLED_STL_HPP_33dhh4hs
#define CONFIG_POOLED_STL_HPP_33dhh4hs

#include <boost/pool/pool_alloc.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>

namespace pooled {
  template<class Contained>
  struct container {
    typedef boost::fast_pool_allocator<Contained> allocator_type;

    typedef std::vector<Contained, allocator_type> vector;
    typedef std::list<Contained, allocator_type> list;
  };

  template<class Key, class Value>
  struct assoc {
    typedef typename std::pair<Key,Value> pair_type;
    typedef boost::fast_pool_allocator<pair_type> allocator_type;

    typedef std::less<Key> compare_type;

    typedef typename std::map<Key, Value, compare_type, allocator_type> map;
  };

  typedef std::basic_string<char, std::char_traits<char>, boost::fast_pool_allocator<char> > string;
}

#endif
