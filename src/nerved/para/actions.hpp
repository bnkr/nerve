// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_monitors
 *
 * Monitor strategies which go in basic_monitor.  These deal with all aspects of
 * synhronisation, while the monitor deals only with the scoping.
 */

#ifndef PARA_ACTIONS_HPP_hwohr6an
#define PARA_ACTIONS_HPP_hwohr6an

#include <boost/type_traits/remove_reference.hpp>

namespace para {
  /*!
   * \ingroup grp_monitors
   * The most simple actions that does blocking infailable locks, waits, and
   * notifies the condition.  The sync can be by value or by reference.  By
   * reference is most useful if there will be multiple instanciations of the
   * actions class, while by value is for if the actions are stored centerally
   * somewhere.  Naturally, you will need to keep only one copy of the sync.
   *
   * Typically this class will be used by the +monitor+, so it is all abstracted
   * and you don't need to care about it too much but this convention is used in
   * all the actions to get the most flexible organisation
   */
  template<class MonitorSync>
  class simple_actions {
    public:
    typedef typename boost::remove_reference<MonitorSync>::type monitor_sync_type;
    typedef typename monitor_sync_type::scoped_lock_type scoped_lock_type;

    explicit simple_actions()
    : lock_(sync_.lockabke()) {}
    explicit simple_actions(monitor_sync_type &s)
    : sync_(s), lock_(sync_.lockable()) {}

    template<class Function>
    void wait(Function data_ready) {
      while (! data_ready()) {
        sync_.waitable().wait(lock_);
      }
    }

    void finish() { sync_.waitable().notify_one(); }

    private:
    // nb: must be allowed to be a reference
    MonitorSync sync_;
    scoped_lock_type lock_;
  };

#if 0
  //! \ingroup grp_monitors
  //! Times the lock but not the wait.
  template<class MonitorSync, class Time>
  class lock_timed_actions {
    public:
    typedef typename boost::remove_reference<MonitorSync>::type monitor_sync_type;

    // TODO:
    //   Specification of Time needs work.

    explicit lock_timed_actions(Time lock_time) {}
    explicit lock_timed_actions(monitor_sync_type &s, Time lock_time) : sync_(s) {}

    void lock() { sync_.lockable().timed_lock(lock_time_); }
    void unlock() { sync_.lockable().unlock(); }
    void wait() { sync_.waitable().wait(sync_.lockable()); }
    void notify() { sync_.waitable().notify(); }

    private:
    MonitorSync sync_;
    Time lock_time_;
  };
#endif
}
#endif
