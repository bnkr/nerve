// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PLAYER_STATE_HPP_nkyeycmj
#define PLAYER_STATE_HPP_nkyeycmj

namespace player {
  //! \ingroup grp_player
  //! Playlist state and other bits.  This is the storage of state and the
  //! gritty bits of changing it from the server and the pipeline.
  class state {
    public:
    typedef const char ** playlist_iterator;

    const char *file_playing();
    playlist_iterator playlist_begin();
    playlist_iterator playlist_end();
  };
}
#endif
