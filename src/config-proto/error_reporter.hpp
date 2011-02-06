// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

namespace config {
  //! \ingroup grp_config
  class error_reporter {
    public:

    error_reporter() : error_(false), fatal_error_(false) {}

    bool error() const { return error_ || fatal_error_; }
    bool fatal_error() const { return fatal_error_; }

    private:
    bool error_;
    bool fatal_error_;
  };
}
