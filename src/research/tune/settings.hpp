/*!
\file
\brief Command-line option parsing.
*/
#ifndef SETTINGS_HPP_g0vr9k40
#define SETTINGS_HPP_g0vr9k40

class settings {
  public:
    // TODO:
    //   better some kind of bitmask.  I want
    //
    //     if i.note() == 'a', freq = whtever
    //
    //   We already have to check the notes exist so it's OK to normalise them here, like a# == bB
    typedef std::string note_type;
    typedef std::vector<note_type> notes_list_type;

    settings(int argc, const char **argv) {
      /// parse args
      /// validate args
    }

    std::size_t duration() const {}
    const notes_list_type &notes() const {}
};

#endif
