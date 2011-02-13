// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef UTIL_POOLED_HPP_fdmqdjzd
#define UTIL_POOLED_HPP_fdmqdjzd

#include <boost/pool/pool_alloc.hpp>

#include <string>
#include <vector>
#include <list>
#include <map>

namespace pooled {
  // struct pool_tag;

  //! \ingroup grp_pooled
  template<class Contained>
  struct container {
    typedef boost::pool_allocator<Contained> contiguous_allocator_type;
    typedef boost::fast_pool_allocator<Contained> object_allocator_type;

    typedef std::vector<Contained, contiguous_allocator_type> vector;
    typedef std::list<Contained, object_allocator_type> list;
  };

  //! \ingroup grp_pooled
  template<class Key, class Value>
  struct assoc {
    typedef typename std::pair<Key,Value> pair_type;
    typedef boost::fast_pool_allocator<pair_type> allocator_type;

    typedef std::less<Key> compare_type;

    typedef typename std::map<Key, Value, compare_type, allocator_type> map;
  };

  //! \ingroup grp_pooled
  typedef std::basic_string<char, std::char_traits<char>, boost::pool_allocator<char> > string;

  //! \ingroup grp_pooled
  template<class T>
  T *alloc() {
    T * const p = (T*) boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(T)>::malloc();
    new (p) T;
    return p;
  }

  //! \ingroup grp_pooled
  template<class T>
  void free(T *ptr) {
    ptr->~T();
    boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(T)>::free(ptr);
  }

  //! Pooling which remembers how much was allocated.
  //@{
  //! \ingroup grp_pooled

  void *tracked_byte_alloc(size_t);
  void tracked_byte_free(void *);
  void *tracked_byte_realloc(void *ptr, size_t bytes);
  //@}
}

#endif