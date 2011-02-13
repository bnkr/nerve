  enum types {
    //! Error with configuration.  This gets put on stderr.
    err_configure
  };

  // meh.. don't like this too much -- would prefer a class which isn't global
  void log(enum types, const char *format, ...);

