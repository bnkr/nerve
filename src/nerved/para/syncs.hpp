// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_syncs
 *
 * Synchronisation primitives.
 */

#ifndef PARA_SYNCS_HPP_h8412an4
#define PARA_SYNCS_HPP_h8412an4

namespace para {
  //! \ingroup grp_syncs
  //! The synchronisation primitives for a monitor.
  template<class Waitable, class Lockable, class ScopedLock = typename Lockable::scoped_lock>
  class basic_monitor_sync {
    public:
    typedef Lockable lockable_type;
    typedef Waitable waitable_type;
    typedef ScopedLock scoped_lock_type;

    lockable_type &lockable() { return lockable_; }
    waitable_type &waitable() { return waitable_; }

    private:
    lockable_type lockable_;
    waitable_type waitable_;
  };
}
#endif
