
    ffmpeg::initialiser ff;

  ffmpeg::packet pkt;
  ffmpeg::file file(file_name);
  // file.dump(); //file_name);
  ffmpeg::audio_stream audio(file);

  ffmpeg::packet_reader pr(pkt, file);
  while (pr.read()) {
    // TODO:
    //   bug - if you drop all frames, then we wait for the exit signal forever.  Fixed now?

    ffmpeg::audio_decoder decoder(audio);
    ffmpeg::decoded_audio decoded(decoder, pkt);

    push_packet(decoded);
  }

