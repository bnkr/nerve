// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_monitors
 *
 * Implementations of the monitor pattern.
 */

#ifndef PARA_MONITORS_HPP_zl6gcv0u
#define PARA_MONITORS_HPP_zl6gcv0u

#include "actions.hpp"

namespace para {
  /*!
   * \ingroup grp_monitors
   *
   * Generalised monitor which relies on the actions to define its semantics.
   * The monitor actions are taken by reference, so you end up with:
   *
   *   sync sync;
   *   actions act(sync);
   *   // in a function
   *   basic_monitor<actions> m(act, &predicate);
   *
   * This design means that you can:
   *
   * - implement lock times (action constructor or wait)
   * - implement wait times (in wait)
   * - implement different notify strategies (in finish)
   * - have a stateful actions calss
   * - use multiple differenent actions but the same sync
   * - keep the "data ready" predicate in the actions
   *
   * In other words: very flexible.
   */
  template<class MonitorActions>
  class basic_monitor {
    public:
    typedef MonitorActions actions_type;

    explicit basic_monitor(actions_type &s) : a_(s) {
      this->actions().wait();
    }

    template<class Function>
    explicit basic_monitor(actions_type &s, Function f) : a_(s) {
      this->actions().wait(f);
    }

    ~basic_monitor() {
      this->actions().finish();
    }

    actions_type &actions() { return a_; }
    const actions_type &actions() const { return a_; }

    private:
    actions_type &a_;
  };

  //! \ingroup grp_monitors
  //! Template meta-function to yield a type for actions dependant on the
  //! monitor sync.
  struct simple_action_selector {
    template<class MonitorSync>
    struct actions { typedef typename para::simple_actions<MonitorSync> type; };
  };

  /*!
   * \ingroup grp_monitors
   * Uses a stateless actions class and takes a reference to a monitor sync.
   */
  template<class MonitorSync, class ActionSelector = simple_action_selector>
  class monitor {
    public:
    typedef MonitorSync monitor_sync_type;
    typedef typename ActionSelector::template actions<monitor_sync_type&>::type actions_type;
    typedef basic_monitor<actions_type> basic_monitor_type;

    template<class Function>
    explicit monitor(monitor_sync_type &sync, Function f)
    : actions_(sync), mon_(actions_, f) { }

    actions_type &actions() { return mon_.actions(); }
    const actions_type &actions() const { return mon_.actions(); }

    private:
    actions_type actions_;
    basic_monitor_type mon_;
  };
}

#endif
