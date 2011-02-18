
    sdl::audio aud;
    sdl::device dev(aud);
    {
      sdl::audio_spec::callback_type cb = sdl_callback;
      sdl::audio_spec desired(cb);
      // std::cout << "Opening audio:" << std::endl;
      // desired.dump_spec(std::cout, "  ") << std::endl;
      dev.reopen(desired);
      std::cout << "SDL Got:" << std::endl;
      dev.obtained().dump_spec(std::cout, "  ") << std::endl;
    }
