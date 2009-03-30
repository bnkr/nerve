/*!
\file
\brief Static data about notes.
*/
#ifndef DATA_HPP_yy1x8uag
#define DATA_HPP_yy1x8uag

namespace notes {
  typedef enum {
    aB  a,
    bB, b,
    c,
    dB, d,
    eB, e,
    f,
    gB, g,
    notes_size
  } notes_type;
}

// note = (note - 1) * 12-root(2);
// `concert pitch' is A which is 440hz.
// So we get
// f = 2^(o/12) * 440Hz
// where o is the number of half-steps away from A.
//
// so how do I actually make the note?  Just sin(value) ?
double freqs[notes_size] = {

  440,  // a
}
//

// Here is how to calculate a sample based on a frequency.  I think the
// 0.75 is a volume changer.  No idea what 32768.0 is.

#if 0
// sample rate
buf_size = format.bits/8 * format.channels * format.rate
freq = freq of note
malloc buffer
for (i = 0; i < format.rate; i++) {
  sample = (int)
  // this is the `calc part of sinwave at x part'.  It would be better
  // if I could explain that value.
    (0.75 * 32768.0 * sin(2 * M_PI * freq * ((float) i/format.rate)));

  /* Put the same stuff in left and right channel */
  buffer[4*i] = buffer[4*i+2] = sample & 0xff;
  buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
}
#endif

#endif
