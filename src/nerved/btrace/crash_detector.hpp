// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_crash
 *
 * Crash detecting context with abstract logging.
 */

#ifndef BTRACE_CRASH_DETECTOR_HPP_x9yjdxam
#define BTRACE_CRASH_DETECTOR_HPP_x9yjdxam

#include <signal.h>
#include <cassert>
#include <boost/noncopyable.hpp>

namespace btrace {
  namespace detail {
    const char *get_signal_name(int);
  }

  struct pretty_backtrace;

  //! \ingroup grp_crash
  //! What happened in a crash.  This is passed to the logger.  Do not query the
  //! fields which aer inapropriate (e.g address if ! memory_fault()) because
  //! you get nonsense values.
  class crash_data {
    public:
    explicit crash_data(siginfo_t *inf, pretty_backtrace &bt)
    : inf_(inf),
      bt_(bt)
    {}

    //! Causing signal
    int signal() const { return inf_->si_signo; }
    //! Textual name for the signal.  Can be null.
    const char *signal_name() const { return detail::get_signal_name(this->signal()); }

    //! Reason for sending of the signal.
    int code() const { return inf_->si_code; }
    //! Text representing code.
    const char *code_name() const;

    //! Was it sent by another process?
    bool from_process() const;
    //! Sending process.
    pid_t sending_pid() const { return inf_->si_pid; }
    //! User id of sending process.
    uid_t sending_uid() const { return inf_->si_uid; }

    //! Is it to do with memory?
    bool memory_fault() const;
    //! What memory caused the problem.  This is the address supplied by
    //! sigaction.  It might not be the same as the top of the backtrace.
    const void *address() const { return inf_->si_addr; }

    //! Many platforms won't be able to produce a backtrace.
    const pretty_backtrace &backtrace() const { return bt_; }

    private:
    siginfo_t *inf_;
    pretty_backtrace &bt_;
  };

  //! \ingroup grp_crash
  //! Interface for generalised logging.
  class crash_logger {
    public:
    virtual ~crash_logger() {}
    virtual void log(const crash_data &) = 0;
  };

  //! \ingroup grp_crash
  //! Namespace for signal masks.
  namespace sig {
    //! \ingroup grp_crash
    enum signal_mask {
      //! Floating point unit exception
      fpe = 1 << 0,
      //! Memory segement violation.
      segv = 1 << 1,
      //! Abort from assertion.
      abrt = 1 << 2,
      //! Bad memory access.
      bus = 1 << 3,
      //! Interupted.
      inter = 1 << 4,
      //! Terminated by process.
      term = 1 << 6,
      //! Illegal operation
      ill = 1 << 7,
    };

    //! \ingroup grp_crash
    typedef int mask_type;

    //! \ingroup grp_crash
    //! Alias for all crashes.
    static const mask_type crashes = fpe | segv | abrt | bus | ill;
  }

  /*!
   * \ingroup grp_crash
   *
   * While in scope, crashes (i.e uncaught signals) will be dealt with as
   * specified by the crash_handlers.  Note that the crash detector works for
   * the entire process.  It is not possible to have different handlers for each
   * thread.
   *
   * If you need different loggers for different signals, then initialise
   * multiple crash detectors.  Take care when using the reset mask functions.
   */
  class crash_detector : boost::noncopyable {
    public:
    explicit crash_detector(sig::mask_type mask = sig::crashes);
    explicit crash_detector(crash_logger *, sig::mask_type mask = sig::crashes);

    //! Restores old handlers.
    ~crash_detector();

    //! Restore early.
    void restore();

    //! Useful if you want basic crash detection (on stderr) while setting up
    //! some more advanced logging framework nad then upgrade later.
    void logger(crash_logger *l) { log_ = l; }
    crash_logger *logger() const { return log_; }

    sig::mask_type mask() const { return mask_; }

    private:
    void construct(crash_logger *, sig::mask_type);
    void enable(crash_logger *, sig::mask_type);

    private:
    struct old_action {
      // the action to be restored
      struct sigaction sig;
      // the detector to be restored (if any)
      crash_detector *detector;
    };

    sig::mask_type mask_;
    crash_logger *log_;
    old_action old_abrt_;
    old_action old_segv_;
    old_action old_fpe_;
    old_action old_inter_;
    old_action old_bus_;
    old_action old_term_;
    old_action old_ill_;
  };

  namespace detail {
    //! Singleton containing mappings loggers.
    class crash_static {
      public:
      void static_init();
      void static_destroy();

      crash_static();

      crash_detector *detector(int);

      // This is necessary to implement reset_logger with no possiblity to
      // overwrite another crash detector's stuff.  Essentially this is just an
      // indirect way of getting to the logger.
      struct {
        crash_detector *abrt;
        crash_detector *segv;
        crash_detector *fpe;
        crash_detector *inter;
        crash_detector *bus;
        crash_detector *ill;
        crash_detector *term;
      } detectors;

      int init_count_;
      void *stack_mem_;
    };

    extern crash_static crash_data;
  }

  //! \ingroup grp_crash
  //! Default logger prints to stderr.
  class console_logger : public crash_logger {
    public:
    void log(const crash_data &data);
  };

  namespace detail {
    extern console_logger default_logger;
  }
}
#endif
