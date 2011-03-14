// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PARA_POINTERS_HPP_egeeiz2d
#define PARA_POINTERS_HPP_egeeiz2d

namespace para {

  //! \ingroup grp_monitors
  //! While in scope, a monitor is active.
  template<class T, class Monitor>
  class monitored_ptr {
    public:
    // TODO:
    //   This is really horrible!  It's necessary only to allow a monitor or a
    //   basic_monitor.
    typedef typename Monitor::init_type init_type;
    monitored_ptr(T &v, init_type &in) : m_(in), v_(v) { }

    T *operator->() { return &v_; }

    private:
    Monitor m_;
    T &v_;
  };
}
#endif
