include_directories(${AVCODEC_PATH} ${AVFORMAT_PATH})

set(RES_DIR "${CMAKE_SOURCE_DIR}/src/research")

set(FFMPEG_LIBS "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}")

set(ALL_IO_LIBS
  ${SDL_LIBRARY}
  ${FFMPEG_LIBS}
)

set(UTILITY_LIBS
  "${BOOST_FILESYSTEM_LIB}" "${BOOST_THREAD_LIB}"
)

if (WIN32)
  # boost.system uses this
  list(APPEND UTILITY_LIBS "${BOOST_SYSTEM_LIB}")
endif()

set(bin_name "gapless-playback")
bbuild_exe(
  TARGET "gapless-playback"
  SOURCES
    "${bin_name}/main.cpp"
    "${bin_name}/play.cpp"
    "${bin_name}/output.cpp"
    "${bin_name}/chunkinate.cpp"
    "${bin_name}/shared_data.cpp"
    "${bin_name}/dump_file.cpp"
    "${bin_name}/degapifier.cpp"
  LIBS ${FFMPEG_LIBS} ${UTILITY_LIBS}
  NO_INSTALL
)

# This has link names like -lpthread, so we can't get it past the validation.
target_link_libraries(gapless-playback ${SDL_LIBRARY})

# tripple-gapless-playback
set(bin_name "tripple-gapless-playback")
add_executable("${bin_name}" "tripple-gapless-playback.cpp")
target_link_libraries("${bin_name}" ${ALL_IO_LIBS} ${UTILITY_LIBS})

# interrupted-playback
set(bin_name "interrupted-playback")
set(src "src/research/interrupted-playback.cpp")
add_executable("${bin_name}" "interrupted-playback.cpp")
target_link_libraries("${bin_name}" ${ALL_IO_LIBS} ${UTILITY_LIBS})
