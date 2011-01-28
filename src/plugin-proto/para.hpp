#include <boost/thread.hpp>

/*
 * Library Overview
 * ----------------
 *
 * This section is a simple overview of the monitor library from the bottom
 * level classes to the top level.
 *
 * All monitors have a +MonitorSync+.  This is mrerely a tuple which binds
 * together the two synchronisation primitices (the +Locakble+ and the
 * +Waitable+).  One could use the +MonitorSync+ to fix some parameters of the
 * types.
 *
 *   struct MonitorSync {
 *     lockable_type &lockable();
 *     condition_type &condition();
 *   };
 *
 * The +MonitorSync+ is stored in a +Monitorable+ type.  This exposes the
 * lockable and waitable methods.  The +Monitorable+ is really just a bridge
 * between the synchronisation primitives and the sequence of calls that make up
 * a monitor.  The +Monitorable+ is used to supply per-invocation parameters.
 *
 *   struct Monitorable {
 *     void lock();
 *     void wait();
 *     void unlock();
 *     void notify();
 *   };
 *
 * It is expected that in the most common usage, there would only be one
 * strategy per monitor, therefore the minotorale models all hold a sync by
 * default.  It is also an option to hold by reference, though.
 *
 * The +Monitor+ is a scoped object which ensures the proper ordering of calls.
 * It takes a +Monitorable+ which decides the strategy for the calls themselves.
 * The monitor operates like a scoped lock.
 *
 *   struct Monitor {
 *      // This requirement helps when the monitor is wrapping some other type
 *      // (like how the +monitor+ class does).  Usually it just means a
 *      // +Monitorable+ or a +MonitorSync+.
 *      Monitor(init_type &);
 *
 *      monitorable_type &monitorable();
 *   };
 *
 * The pointer types respect unique pointer semantics where the object pointed
 * to is monitored while the pointer is in scope.  The pointer holds the monitor
 * type by value.
 *
 * Monitor Classes
 * ---------------
 *
 * The +monitor+ class is the most simple usage.  It represents using a single
 * locking strategy.
 *
 * ---
 * struct C {
 *   typedef para::default_monitor_sync sync_type;
 *   typedef para::monitor<sync_type> monitor_type;
 *
 *   void use() {
 *     monitor_type m(sync_);
 *     // thread safe now
 *   }
 *
 *   private:
 *     sync_type sync_;
 * };
 * ---
 *
 * The +monitor+ is based on the +basic_monitor+ template.  It uses that class
 * with a default +Monitorable+.  The following example is equivilent to the
 * above:
 *
 * ----
 * struct C {
 *   typedef para::default_monitor_sync sync_type;
 *   typedef para::simple_monitorable<sync_type> monitorable_type;
 *   typedef para::basic_monitor<monitorable_type> monitor_type;
 *
 *   C() : monitorable_(sync_) {}
 *
 *   void use() {
 *     monitor_type m(monitorable_);
 *     // thread safe now
 *   }
 *
 *   // this one has a different strategy
 *   void another_use() {
 *     para::timed_monitorable strat_(sync_, time(2));
 *     para::monitor<para::timed_monitorable> m(strat_);
 *     // thread safe now
 *   }
 *
 *   private:
 *     sync_type sync_;
 *     monitorable_type monitorable_;
 * };
 * ----
 *
 * MonitorSync Types
 * -----------------
 *
 * All obvious:
 *
 * - basic_monitor_sync<Conditon, Lockable>;
 *
 * Monitorable Types
 * -----------------
 *
 * The monitorable classes' usage is all along the lines of the aboce examples.
 *
 * - simple_monitorable
 * - lock_timed_monitorable
 * - wait_timed_monitorable
 * - timed_monitorable
 *
 * Pointer Types
 * -------------
 *
 * The pointers are all used in the standard way expected for smart pointers.
 * The make use of the +init_type+ member of the +Monitor+ concept so the
 * monitors which directly take a +Monitorable+ or wrap a +MonitorSync+ in its
 * own +Monitorable+.
 *
 *   object o;
 *   monitor_ptr<object, monitor<> > ptr(o);
 *   // thread safe now
 *   pth->something();
 *
 */

// monitor_syncs.hpp
namespace para {
  //! The synchronisation primitives for a monitor.
  template<class Condition, class Lockabke>
  class basic_monitor_sync {
    public:
      Lockable &lockable() { return mutex_; }
      Condition &condition() { return condition_; }

    private:
      Lockabke mutex_;
      Conditon cond_;
  };

  //! The most simple monitorable that does blocking infailable locks, waits, and
  //! notifies the condition.
  template<class MonitorSync>
  class simple_monitorable {
    // TODO:
    //   THe class and concept name is probably not very sensible.

    typedef typename boost::remove_reference<MonitorSync>::type monitor_sync_type;

    explicit simple_monitorable() {}
    explicit simple_monitorable(monitor_sync_type &s) : sync_(s) {}

    public:
      void lock() { sync_.lockable().lock(); }
      void unlock() { sync_.lockable().unlock(); }
      void wait() { cond_.waitable().wait(l); }
      void notify() { cond_.waitable().notify(); }

    private:
      MonitorSync sync_;
  };

  //! Times the lock but not the wait.
  template<class MonitorSync>
  class lock_timed_monitorable {
    public:
      typedef typename boost::remove_reference<MonitorSync>::type monitor_sync_type;

      // TODO:
      //   It's a bit annoying to write double the constructors all the time...
      explicit simple_monitorable(time lock_time) {}
      explicit simple_monitorable(monitor_sync_type &s, time lock_time) : sync_(s) {}

      void lock() { sync_.lockable().timed_lock(lock_time_); }
      void unlock() { sync_.lockable().unlock(); }
      void wait() { cond_.waitable().wait(sync_.lockable()); }
      void notify() { cond_.waitable().notify(); }

    private:
      MonitorSync sync_;
      time lock_time_;
  };
}

// monitors.hpp
namespace para {
  //! Generalised monitor which relies on the monitorable to define its semantics.
  //! The lock and wait
  template<class Monitorable>
  class basic_monitor {
    public:
      typedef Monitorable monitorable_type;

      monitor(monitorable_type &m) : m_(m) {
        this->monitorable().lock();
        this->monitorable().wait();
      }

      ~monitor() {
        this->monitorable().notify();
        this->monitorable().unlock();
      }

      monitorable_type &monitorable() { return m_; }
      const monitorable_type &monitorable() const { return m_; }

    private:
      Monitorable &sync_;
  };

  //! Uses the +simple_monitorable+ to make a monitor which does a basic lock,
  //! wait, and notify.
  template<class MonitorSync>
  class monitor {
    public:
      typedef monitor_sync_type init_type;

      typedef MonitorSync monitor_sync_type;
      typedef simple_monitorable<monitor_sync_type&> monitorable_type;
      typedef basic_monitor<monitorable_type> basic_monitor_type;

      monitor(monitor_sync_type &sync)
      : strat_(sync), mon_(strat_) { }

      monitorable_type &monitorable() { return mon_.monitorable(); }
      const monitorable_type &monitorable() const { return mon_.monitorable(); }

    private:
      monitorable_type strat_;
      basic_monitor_type mon_;
  };
}

// pointers.hpp
namespace para {
  //! A pointer to data protected by a monitor.
  template<class T, class Monitor>
  class monitored_ptr {
    public:
      typedef Monitor::init_type init_type;
      monitored_ptr(T &v, init_type &in) : m_(in), v_(v) { }

      T *operator->() { return &v_; }

    private:
      Monitor m_;
      T &v_;
  };

}
