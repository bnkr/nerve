// Copyright (c) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "crash_detector.hpp"
#include "backtrace.hpp"

#include "arch.hpp"

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

using namespace btrace;

//! Just in case we fall back to ansi signals.  The third argument isa
//! ucontext_t.
typedef void(*handler_type)(int,siginfo_t*,void*);

//! Sigaction handler.
static void signal_handler(int, siginfo_t *, void *);

//! Low-level modifiers.
static void change_handler(int sig, const struct sigaction &act, struct sigaction &old);
static void restore_handler(int sig, const struct sigaction &act);

detail::crash_static detail::crash_data;
console_logger detail::default_logger;

/**************************
 * Constructor/Destructor *
 **************************/

crash_detector::crash_detector(sig::mask_type m) {
  construct(&detail::default_logger, m);
}

crash_detector::crash_detector(crash_logger *log, sig::mask_type m) {
  construct(log, m);
}

void crash_detector::construct(crash_logger *log, sig::mask_type m) {
  detail::crash_data.static_init();
  mask_ = m;
  log_ = log;
  enable(log_, mask_);
}

crash_detector::~crash_detector() {
  restore();
  detail::crash_data.static_destroy();
}

/********************
 * Generic Changers *
 ********************/

#define OLD_VAR(sig__) this->old_ ## sig__ ## _
#define DETECTOR_VAR(sig__) detail::crash_data.detectors.sig__

#define FOR_ALL_SIGNALS(name__, mask__, ...)\
  name__(mask__, abrt, SIGABRT, __VA_ARGS__);\
  name__(mask__, segv, SIGSEGV, __VA_ARGS__);\
  name__(mask__, fpe, SIGFPE, __VA_ARGS__);\
  name__(mask__, bus, SIGBUS, __VA_ARGS__);\
  name__(mask__, inter, SIGINT, __VA_ARGS__);\
  name__(mask__, term, SIGTERM, __VA_ARGS__);\
  name__(mask__, ill, SIGILL, __VA_ARGS__);

//! Sets the signal handler and logger for the mask.  No restoration but stores
//! the old handlers etc.
void crash_detector::enable(crash_logger *log, sig::mask_type m) {
  struct sigaction act;
  act.sa_flags =
    // use alt stack
    SA_ONSTACK |
    // take the extended handler
    SA_SIGINFO  |
    // make some system call restartable (for BSD compatibility)
    SA_RESTART;
  act.sa_sigaction = &signal_handler;
  // Block all other signals.
  sigfillset(&act.sa_mask);

  // TODO:
  //   A non-reentrant bit.  Could be made thread-safe with a lock that the
  //   crash handler respects?  Also consider if a signal arrives from another
  //   thread.

#define SET_SIG(mask__, sig__, num__, act__)\
  if (mask__ & sig::sig__) {\
    OLD_VAR(sig__).detector = DETECTOR_VAR(sig__);\
    change_handler(num__, act__, OLD_VAR(sig__).sig);\
    DETECTOR_VAR(sig__) = this;\
  }

  FOR_ALL_SIGNALS(SET_SIG, m, act);
}

//! Restores the stored behavior for given mask.
void crash_detector::restore() {
  // TODO:
  //   Also a race here.
#define RESTORE(mask__, sig__, num__, ignored)\
  if (mask__ & sig::sig__) {\
    restore_handler(num__, OLD_VAR(sig__).sig);\
    DETECTOR_VAR(sig__) = OLD_VAR(sig__).detector;\
  }

  FOR_ALL_SIGNALS(RESTORE, this->mask_);
}

/***************
 * Static Data *
 ***************/

crash_detector *detail::crash_static::detector(int sig) {
 switch (sig) {
  case SIGABRT:
    return detectors.abrt;
  case SIGSEGV:
    return detectors.segv;
  case SIGBUS:
    return detectors.bus;
  case SIGINT:
    return detectors.inter;
  case SIGFPE:
    return detectors.fpe;
  case SIGTERM:
    return detectors.term;
  case SIGILL:
    return detectors.ill;
  default:
    std::cerr << "btrace: crash_detector: no logger found -- this is a bug" << std::endl;
    return NULL;
  }
}

detail::crash_static::crash_static() : init_count_(0), stack_mem_(0) {
  std::memset(&detectors, 0, sizeof(detectors));
}

void detail::crash_static::static_init() {
  if (init_count_ == 0) {
    const size_t size = 4096;
    stack_t new_stack;
    new_stack.ss_flags = 0;
    new_stack.ss_sp = this->stack_mem_ = std::malloc(size);
    new_stack.ss_size = size;
    if (::sigaltstack(&new_stack, NULL) != 0) {
      std::cerr << "btrace: crash_detector: unable to establish an alternate signal stack" << std::endl;
    }
  }
  ++init_count_;
}

void detail::crash_static::static_destroy() {
  if (init_count_ == 1) std::free(this->stack_mem_);
  --init_count_;
}

/***********************
 * Low-level modifiers *
 ***********************/

void change_handler(int sig, const struct sigaction &act, struct sigaction &old) {
  if (::sigaction(sig, &act, &old) != 0) {
    const char *const e = std::strerror(errno);
    std::cerr << "btrace: crash_detector: warning: unable to change signal ";

    const char *const n = detail::get_signal_name(sig);
    if (n) {
      std::cerr << n << "(" << sig << "): ";
    }
    else {
      std::cerr << sig;
    }

    std::cerr << e << std::endl;
  }
}

void restore_handler(int sig, const struct sigaction &act) {
  if (::sigaction(sig, &act, NULL) != 0) {
    std::cerr << "btrace: crash_detector: warning: unable to restore signal " << sig << std::endl;
  }
}

/***************
 * Signal name *
 ***************/

static const char * const sig_table[] =
#if defined(BTRACE_ARCH_ALPHA) || defined(BTRACE_ARCH_SPARC)
    {
      "zero?!",
      "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
      "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", // 10
      "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM",
      "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", // 20
      "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ",
      "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPWR", "SIGUSR1", // 30
      "SIGUSR2"
    };
#elif defined(BTRACE_ARCH_MIPS)
    {
      "zero?!",
      "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
      "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", // 10
      "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM",
      "SIGUSR1", "SIGUSR2", "SIGCHLD", "SIGPWR", "SIGWINCH", // 20
      "SIGURG", "SIGIO", "SIGSTOP", "SIGTSTP", "SIGCONT",
      "SIGTTIN", "SIGTTOU", "SIGVTALRM", "SIGPROF", "SIGXCPU", // 30
      "SIGXFSZ"
    };
#else
#  if ! (defined(BTRACE_ARCH_ARM) || defined(BTRACE_ARCH_SH) || defined(BTRACE_ARCH_SYSTEM390) || \
        defined(BTRACE_ARCH_IA64) || defined(BTRACE_ARCH_PPC) || defined(BTRACE_ARCH_X86))
#    warning "The signal name table is unknown for your architecure (guessing x86-like)."
#  endif

/* ix86, ia64, ppc, s390, arm and sh */
    {
      "zero?!",
      "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
      "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", // 10
      "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM",
      "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", // 20
      "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ",
      "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR" // 30
    };
#endif

const char *detail::get_signal_name(int s) {
  assert(s > 0);
  if ((size_t) s >= sizeof(sig_table)) {
    return NULL;
  }
  else {
    return sig_table[s];
  }
}

/**************
 * Crash data *
 **************/

const char *crash_data::code_name() const {
  if (this->signal() == SIGILL) {
    switch (this->code()) {
    case ILL_ILLOPC:
      return "illegal opcode";
    case ILL_ILLOPN:
      return "illegal operand";
    case ILL_ILLADR:
      return "illegal addressing mode";
    case ILL_ILLTRP:
      return "illegal trap";
    case ILL_PRVOPC:
      return "privileged opcode";
    case ILL_PRVREG:
      return "privileged register";
    case ILL_COPROC:
      return "coprocessor error";
    case ILL_BADSTK:
      return "internal stack error";
    }
  }
  else if (this->signal() == SIGFPE) {
    switch (this->code()) {
    case FPE_INTDIV:
      return "integer divide by zero";
    case FPE_INTOVF:
      return "integer overflow";
    case FPE_FLTDIV:
      return "floating-point divide by zero";
    case FPE_FLTOVF:
      return "floating-point overflow";
    case FPE_FLTUND:
      return "floating-point underflow";
    case FPE_FLTRES:
      return "floating-point inexact result";
    case FPE_FLTINV:
      return "floating-point invalid operation";
    case FPE_FLTSUB:
      return "subscript out of range";
    }
  }
  else if (this->signal() == SIGSEGV) {
    switch (this->code()) {
    case SEGV_MAPERR:
      return "address not mapped to object";
    case SEGV_ACCERR:
      return "invalid permissions for mapped object";
    }
  }
  else if (this->signal() == SIGBUS) {
    switch (this->code()) {
    case BUS_ADRALN:
      return "invalid address alignment";
    case BUS_ADRERR:
      return "nonexistent physical address";
    case BUS_OBJERR:
      return "object-specific hardware error";
    // Only on linux (but it seems it's not defined in debian...).
    // case BUS_MCEERR_AR:
    //   return "Hardware memory error consumed on a machine check; action required.";
    // case BUS_MCEERR_AO:
    //   return "Hardware memory error detected in process but not consumed; action optional.";
    }
  }

  switch (this->code()) {
  case SI_USER:
    return "kill(2) or raise(3)";
  case SI_KERNEL:
    return "sent by the kernel";
  case SI_QUEUE:
    return "sigqueue(2)";
  case SI_TIMER:
    return "POSIX timer expired";
  case SI_MESGQ:
    return "POSIX message queue state changed";
  case SI_ASYNCIO:
    return "AIO completed";
  case SI_SIGIO:
    return "queued SIGIO";
  case SI_TKILL:
    return "tkill(2) or tgkill(2)";
  }

  return NULL;
}

bool crash_data::from_process() const {
  // TODO:
  //   Might not be sufficiant.  Also why no tkill?
  return this->code() == SI_USER
    || this->code() == SI_QUEUE;
}

bool crash_data::memory_fault() const {
  switch (this->signal()) {
  case SIGILL:
  case SIGFPE:
  case SIGSEGV:
  case SIGBUS:
  case SIGTRAP:
    return true;
  default:
    return false;
  }
}

/******************
 * Signal Handler *
 ******************/

void console_logger::log(const crash_data &d) {
  std::cerr << "** SIGNAL CAUGHT **" << std::endl;
  const char *sn = d.signal_name();
  const char *cn = d.code_name();
  sn = sn ? sn : "unknown signal";
  cn = cn ? cn : "unknown reason";
  std::cerr << "Signal: " << sn << " (" << d.signal() << ")" << std::endl;
  // TODO:
  //   This does not ever seem to report sensible values.
  std::cerr << "Reason: " << cn  << " (" << d.code() << ")" << std::endl;
  if (d.memory_fault()) {
    // TODO:
    //   This does not seem to work properly.  E.g access to 0x00 doesn't appear
    //   here.  Perhaps it means address of instruction?  In which case we need
    //   to mess with the ucontext.
    std::cerr << "Address: " << d.address() << std::endl;
  }

  if (d.from_process()) {
    std::cerr << "Sender: pid = " << d.sending_pid() << ", uid = " << d.sending_uid() << std::endl;
  }

  if (! d.backtrace().empty()) {
    std::cerr << "Backtrace:" << std::endl;

    typedef pretty_backtrace::iterator iter_t;

    for (iter_t i = d.backtrace().begin(); i != d.backtrace().end(); ++i) {
      std::cerr << "* " << i->symbol() << std::endl;
      std::cerr << "  " << i->file() << ":" << i->line() << std::endl;
    }
  }
  else {
    std::cerr << "Backtrace: unavailable." << std::endl;
  }
}

void signal_handler(int sig, siginfo_t *inf, void *) {
  pretty_backtrace cb;
  crash_data dt(inf, cb);

  crash_detector *const d = detail::crash_data.detector(sig);
  if (d) {
    d->logger()->log(dt);
  }

  // Seems to be the only reliable way to re-raise the signal do it (but I
  // should use sigaction, really).  Then I should restore my funky error
  // handler (just in case the default doesn't quit).
  signal(sig, SIG_DFL);
  raise(sig);
}

