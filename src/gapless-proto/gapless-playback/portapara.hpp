// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * This file is enough bits from the para library that I can compile this
 * program.  It should all be deleted when this prototype is finished.
 */

#include <boost/utility.hpp>
#include <boost/thread.hpp>


// Copyright (C) 2008-2009, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
\file
\brief Base sync tuple structures and adaptor patterns.
*/

#ifndef PARA_LOCKING_TUPLES_HPP_gvbbxrz8
#define PARA_LOCKING_TUPLES_HPP_gvbbxrz8

namespace para {
  namespace detail {
    //! \brief Plugs boost's types into the monitor types.
    struct boost_monitor_traits {
      typedef boost::defer_lock_t     defer_lock_type;
      typedef boost::try_to_lock_t    try_to_lock_type;
      typedef boost::adopt_lock_t     adopt_lock_type;
      typedef boost::system_time      absolute_time_type;
    };

    typedef boost_monitor_traits default_monitor_traits;
  }

  using boost::defer_lock;
  using boost::try_to_lock;
  using boost::adopt_lock;
  template <class Lock, class Condition, class MonitorTraits = detail::default_monitor_traits>
  class deferred_monitor : boost::noncopyable {
    public:
      //! \name Template parameter type aliases.
      //@{
      typedef Lock lock_type;
      typedef Condition condition_type;
      //@}

      //! \name Monitor traits types abbreviated.
      //@{
      typedef typename MonitorTraits::defer_lock_type    defer_lock_type;
      typedef typename MonitorTraits::try_to_lock_type   try_to_lock_type;
      typedef typename MonitorTraits::adopt_lock_type    adopt_lock_type;
      //@}

      //! \name Time type selectors.
      //@{
      struct deadline_time_selector {};
      struct duration_time_selector {};
      //@}

      //! \name Constructors/Destructors
      //@{
      //! Initialise the lock and the wait condition for a later head() call.
      template <class Sync>
      explicit deferred_monitor(Sync &s) : lock_(s.mutex()), cond_(s.wait_condition()) {}

      //! Initialise the lock and condition, but defer the locking until later.
      template <class Sync>
      deferred_monitor(Sync &s, defer_lock_type v) : lock_(s.mutex(), v), cond_(s.wait_condition()) {}

      //! Initialise the lock and condition and use the try lock constructor.
      template <class Sync>
      deferred_monitor(Sync &s, try_to_lock_type v) : lock_(s.mutex(), v), cond_(s.wait_condition()) {}

      //! Initialise the lock and condition and adopt a lock which is already owned by this thread.
      template <class Sync>
      deferred_monitor(Sync &s, adopt_lock_type v) : lock_(s.mutex(), v), cond_(s.wait_condition()) {}

      //! A duration for the *lock*.
      template <class Sync, class Duration>
      deferred_monitor(Sync &s, const Duration &d, duration_time_selector)
      : lock_(s.mutex(), d), cond_(s.wait_condition()) {}

      //! A deadline for the lock.
      template <class Sync, class AbsoluteTime>
      deferred_monitor(Sync &s, const AbsoluteTime &t, deadline_time_selector)
      : lock_(s.mutex(), t), cond_(s.wait_condition()) {}

      //! Release the lock.
      ~deferred_monitor() {}
      //@}

      //! \name Implementing the monitor head, returning false if there was a timeout.
      //@{

      //! Wait on the condition while predicate is NOT true.
      template <class Predicate>
      bool head(Predicate continue_pred) {
        while (! continue_pred()) {
          cond_.wait(lock_);
        }
        return true;
      }

      //! Perform a timed wait using a duration type.
      template <class Predicate, class Duration>
      bool head(Predicate continue_pred, const Duration &d, duration_time_selector) {
        return timed_head(continue_pred, d);
      }

      //! Timed wait with a deadline time.
      template <class Predicate, class AbsoluteTime>
      bool head(Predicate continue_pred, const AbsoluteTime &t, deadline_time_selector) {
        return timed_head(continue_pred, t);
      }

      //@}

      //! \name Accessors
      //@{

      //! \brief The condition which will be waited on.
      Condition &wait_condition() { return cond_; }
      const Condition &wait_condition() const { return cond_; }

      //! \brief The lock which is providing mutual exclusion during this instance's lifetime.
      //! Use this to get at the mutex.
      Lock &lock() { return lock_; }
      const Lock &lock() const { return lock_; }

      //@}

    protected:
      //! Timed wait.  Wrapper which just ignored our selector parameters.
      template <class Predicate, class Time>
      bool timed_head(Predicate continue_pred, const Time &t) {
        bool ok = true;
        while (! continue_pred()) {
          ok = cond_.timed_wait(lock_, t);
        }
        return ok;
      }

      //! \name Checked initialisers for subclasses
      //@{
      template <class LockError, class Predicate>
      void checked_head_failable_lock(Predicate cont_pred) {
        if (! lock().owns_lock()) throw LockError();
        head(cont_pred);
      }

      template <class LockError, class CondError, class Predicate>
      void checked_head_all_failable(Predicate cont_pred) {
        if (! lock().owns_lock()) throw LockError();
        if (! head(cont_pred)) throw CondError();
      }

      template <class LockError, class CondError, class Predicate, class Time>
      void checked_head_all_failable_timed(Predicate cont_pred, const Time &time) {
        if (! lock().owns_lock()) throw LockError();
        if (! timed_head(cont_pred, time)) throw CondError();
      }

      template <class CondError, class Predicate, class HeadParam>
      void checked_head_failable_wait(Predicate cont_pred, HeadParam time) {
        if (! timed_head(cont_pred, time)) throw CondError();
      }
      //@}

    private:
      Lock lock_;
      Condition &cond_;
  };


  /*!
  \ingroup grp_monitors
  \brief A simple monitor which waits on construction.

  This class is not intended for locks and conditions which can fail since the monitor
  head is called immediately, however, using lock models which throw exceptions will
  work.

  It could also be used for upgradable locks due to the mutex/lock accessors, providing
  the condition model supports it.
  */
  template <class Lock, class Condition, class MonitorTraits = detail::default_monitor_traits>
  class monitor : protected deferred_monitor<Lock,Condition,MonitorTraits> {
    typedef deferred_monitor<Lock,Condition,MonitorTraits> base;

    public:
      using base::condition_type;
      using base::lock_type;

      //! \brief Lock and wait on the condition while (! continue_predicate()).
      template <class Sync, class Predicate>
      monitor(Sync &s, Predicate continue_predicate) : base(s) {
        head(continue_predicate);
      }

      using base::lock;
      using base::wait_condition;
  };
  //! \ingroup grp_tuples
  //! \brief T which is guarded by a lockable (which is always mutable).
  //!
  //! T can be a reference, but only on gcc ~=4.4.3.  mingw 4.2 can't deal with
  //! it.
  //
  //TODO:
  //  Fix this -- it'll take ages I bet, but I think the type traits lib has
  //  enough to work around this.
  template <class T, class Mutex>
  class sync_tuple {
    public:
      // TODO: mutex_type is more obvious, but less accurate :)
      typedef Mutex lockable_type;
      typedef T     value_type;

      sync_tuple(const value_type &data = value_type()) : data_(data) {}

      lockable_type &mutex() { return mutex_; }
      const value_type &data() const { return data_; }
      value_type &data() { return data_; }

    private:
      mutable lockable_type mutex_;
      value_type data_;
  };

  //! \ingroup grp_tuples
  //! \brief Adds a condition to a \link sync_tuple \endlink.
  //!
  //! Use this to bind multiple condition variables to one synchronised
  //! data tuple.
  //!
  //! If you only need one condition, then \link monitor_tuple \endlink is
  //! just as good and quicker to write.
  template <class SyncType, class Condition>
  class monitor_bind {
    public:
      typedef typename SyncType::lockable_type lockable_type;
      typedef typename SyncType::value_type    value_type;
      typedef Condition               condition_type;

      monitor_bind(SyncType &sync) : sync_(sync) {}

      value_type &data() { return sync_.data(); }
      const value_type &data() const { return sync_.data(); }
      lockable_type &mutex() { return sync_.mutex(); }
      condition_type &wait_condition() { return cond_; }

    private:
      SyncType &sync_;
      mutable Condition cond_;
  };

  //! \ingroup grp_tuples
  //! \brief A composed \link sync_tuple \endlink and a \link monitor_bind \endlink.
  template<class T, class Mutex, class Condition>
  class monitor_tuple {
    public:
      typedef sync_tuple<T, Mutex>                     sync_tuple_type;
      typedef monitor_bind<sync_tuple_type, Condition> monitor_bind_type;

      typedef typename monitor_bind_type::condition_type condition_type;
      typedef typename monitor_bind_type::lockable_type  lockable_type;
      typedef typename monitor_bind_type::value_type     value_type;

      monitor_tuple(const T &data = T()) : sync_(data), monitor_sync_(sync_) {}

      const T &data() const { return monitor_sync_.data(); }
      T &data() { return monitor_sync_.data(); }
      lockable_type &mutex() { return monitor_sync_.mutex(); }
      condition_type &wait_condition() { return monitor_sync_.wait_condition(); }

    private:
      sync_tuple_type sync_;
      monitor_bind_type monitor_sync_;
  };

  namespace detail {
    //! \brief Created by \link get_monitor_adaptor() \endlink.
    template<class Mutex, class Condition>
    struct monitor_tuple_adaptor {
      typedef Mutex lockable_type;
      typedef Condition condition_type;

      Mutex &m;
      Condition &c;

      monitor_tuple_adaptor(Mutex &m, Condition &c) : m(m), c(c) {}
      Mutex &mutex() { return m; }
      Condition &wait_condition() { return c; }
    };

    //! \brief As \link ::para::detail::monitor_tuple_adaptor monitor_tuple_adaptor \endlink.
    template<class Mutex>
    struct sync_tuple_adaptor {
      Mutex &m;
      sync_tuple_adaptor(Mutex &m) : m(m) {}
      Mutex &mutex() { return m; }
    };
  }

  /*!
  \ingroup grp_locking
  \brief Returns a minimal monitor_tuple concept to initialise a monitor.

  This is to facilitate the use of there monitor patterns without using the sync framework-ish
  classes.
  */
  template <class Mutex, class Condition>
  detail::monitor_tuple_adaptor<Mutex,Condition> get_monitor_adaptor(Mutex &m, Condition &c) {
    return detail::monitor_tuple_adaptor<Mutex,Condition>(m,c);
  }

  //! \ingroup grp_locking
  //! \brief As \link para::get_monitor_adaptor() \endlink, for sync_tuple.
  template <class Mutex>
  detail::sync_tuple_adaptor<Mutex> get_sync_adaptor(Mutex &m) {
    return detail::sync_tuple_adaptor<Mutex>(m);
  }
}

#endif


#ifndef PARA_LOCKING_TRAITS_HPP_p16eecnl
#define PARA_LOCKING_TRAITS_HPP_p16eecnl


namespace para {
  //! \ingroup grp_locking
  //! \brief Meta-programming class to generate the correct classes for a synchronisation tuple.
  //
  //TODO:
  //  First, traits is not really the right word.
  //
  //  Second, we need template typedefs to do this properly.  Perhaps a better
  //  way would be to:
  //
  //  - have this as part of the sync_tuple
  //    - tuples can typedef to this anyway
  //    - so: the todo is link to the traits class  from here.
  //  - maybe I should have sync_traits<Tuple> and sync_types<T,...> ?
  //    - nice idea, but the names are too similar
  //    - also the implementations are identical, I think.
  //
  //  sync_traits<my_sync_type> is much more pretty than the current way.  But
  //  it's a problem for the sync vs. monitor sync concepts because sync doesn't
  //  have anything.  Maybe I need sync_traits, monitor_sync_traits, and keep
  //  this as meta_types.
  //
  //  I could do my_sync_type::sync_types_type -- that is surely even nicer.
  //
  //  Third, the name is wrong.  It should be types.hpp, and
  //  sync_types<whatever>.
  template<class T, class Mutex, class Lock, class Condition>
  struct sync_traits {
    //! \name Synchronization types.
    //@{
    typedef T         value_type;
    typedef Mutex     lockable_type;
    typedef Lock      lock_type;
    typedef Condition condition_type;
    //@}

    //! \name Sync Tuple Types
    //@{
    typedef sync_tuple<value_type, lockable_type>                    sync_tuple_type;
    typedef monitor_bind<sync_tuple_type, condition_type>            monitor_bind_type;
    typedef monitor_tuple<value_type, lockable_type, condition_type> monitor_tuple_type;
    //@}

    //! \name Monitors
    //@{
    typedef monitor<lock_type, condition_type> monitor_type;
    //@}

    //! \name Timed Monitors
    //@{

    //@}

  };
}


#endif
