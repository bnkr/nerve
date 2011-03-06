// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef UTIL_INDIRECT_HPP_ub2ccmil
#define UTIL_INDIRECT_HPP_ub2ccmil

#include "../util/pooled.hpp"

#include <boost/utility.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/function.hpp>
#include <boost/type_traits/remove_pointer.hpp>

//! \ingroup grp_util
//! Used to call the pooled::free function without having to store an explicit
//! funcptr.
struct pooled_destructor {
  template<class T> void operator()(T *const p) const { pooled::free<T>(p); };
};

//! \ingroup grp_util
//! Decorates a container which contains pointers so that it appears to contain
//! references.
template<class Container, class Destructor = boost::function<void(void*)> >
class indirect_owned : boost::noncopyable {
  public:
  typedef Container container_type;
  typedef Destructor destructor_type;

  typedef typename boost::remove_pointer<typename container_type::value_type>::type value_type;
  typedef typename container_type::value_type pointer_type;
  typedef typename container_type::size_type size_type;

  typedef boost::indirect_iterator<typename container_type::iterator> iterator;
  typedef boost::indirect_iterator<typename container_type::const_iterator> const_iterator;

  indirect_owned() {}
  indirect_owned(destructor_type d) : d_(d) {}

  ~indirect_owned() { std::for_each(c_.begin(), c_.end(), d_); }

  bool empty() const { return c_.empty(); }
  size_type size() const { return c_.size(); }

  iterator begin() { return iterator(c_.begin()); }
  iterator end() { return iterator(c_.end()); }

  const_iterator begin() const { return const_iterator(c_.begin()); }
  const_iterator end() const { return const_iterator(c_.end()); }

  void push_back(pointer_type p) { c_.push_back(p); }
  value_type &back() { *c_.back(); }

  private:
  container_type  c_;
  destructor_type d_;
};

namespace detail {
  //! A matching destructor for allocation done in using tracked alloc (which is
  //! necesary because the stages can be differing sizes).
  template<class T>
    struct tracked_destructor {
      void operator()(T *const p) { pooled::tracked_byte_free(p); }
    };
}

//! \ingroup grp_util
//! Owned polymorphic classes where allocation is abstracted.
template<class T>
struct indirect_owned_polymorph :
  protected
  ::indirect_owned<
    typename pooled::container<T*>::vector,
    detail::tracked_destructor<T>
  >
{

  typedef ::indirect_owned<
    typename pooled::container<T*>::vector,
    typename detail::tracked_destructor<T>
  > parent_type;

  typedef typename parent_type::iterator iterator;
  typedef typename parent_type::const_iterator const_iterator;
  typedef typename parent_type::size_type size_type;
  typedef typename parent_type::pointer_type pointer_type;
  typedef typename parent_type::value_type value_type;

  using parent_type::begin;
  using parent_type::end;

  using parent_type::empty;
  using parent_type::size;

  //! Allocate a polymorphic relation of T and push it to the back of the
  //! container
  template<class U>
  U *alloc_back() {
    U* const p = (U*) pooled::tracked_byte_alloc(sizeof(U));
    new (p) U();
    push_back(p);
    return p;
  }
};

#endif
