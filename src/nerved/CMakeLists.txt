# Copyright (C) 2011, James Webber.
# Distributed under a 3-clause BSD license.  See COPYING.

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(is_debug 1)
else()
  set(is_debug 0)
endif()

binfo_find_lib(
  VAR DL_LIB VAR_DOC "Interface to the dynamic linker"
  TASK nerved TASK_DOC "Used under debug configurations to get nicer backtraces."
  NAME dl
)

binfo_find_lib(
  VAR BOOST_THREAD_LIB VAR_DOC "Interface to the dynamic linker"
  TASK nerved TASK_DOC "Used under debug configurations to get nicer backtraces."
  NAMES boost_thread-mt boost_thread
)

binfo_find_lib(
  VAR BOOST_SYSTEM_LIB VAR_DOC "boost.system library"
  TASK nerved TASK_DOC "Boost's error checking library used by boost.asio and possilbly other bits."
  NAMES boost_system-mt boost_system
)

binfo_find_lib(
  NAMES avutil
  VAR AVUTIL_LIB
  VAR_DOC "Ffmpeg's utility library."
)

binfo_find_lib(
  NAMES avcodec
  VAR AVCODEC_LIB
  VAR_DOC "Ffmpeg's encoding/decoding library."
)

binfo_find_lib(
  NAMES avformat
  VAR AVFORMAT_LIB
  VAR_DOC "Ffmpeg's file format library."
)

# FindSDL doesn't work properly with crosscomp due to the use of sdl-config
# binary.
if (NOT CMAKE_CROSS_COMPILING)
  # defines SDL_LIBRARY and all the stuff from sdl-config bin.
  find_package(SDL)
else()
  message("Warning: SDL library can't be auto-detected when cross-compiling.  You may need")
  message("         to add additional cflags and include directories.")
  find_library(SDL_LIBRARY "SDL")
endif()


# TODO:
#   This doesn't work because you need to give extra PATHS.
# binfo_find_dir(
#   VAR "AVUTIL_H_PATH" VAR_DOC "Path to ffmpeg's avutil.h"
#   NAME "avutil.h"
#   TASK "nerved"
#   TASK_DOC "FFmpeg's avutil include dir.  Distros put ffmpeg in differend places so this path is used to change the -I flags."
# )

foreach (file avcodec avformat)
  string(TOUPPER "${file}" file_uc)
  set(cache_var "${file_uc}_H_PATH")
  find_path(
    "${cache_var}"
    NAMES "${file}.h"
    DOC "Path to ffmpeg's ${file}.h"
    PATHS
      "/usr/include/ffmpeg/lib${file}"
      "/usr/include/ffmpeg"
      "/usr/include/lib${file}"
  )

  binfo_set_task_docs(
    CACHE_VAR "${cache_var}"
    TASK "nerved"
    TASK_DOC
      "FFmpeg's ${file} include dir.  Distros put ffmpeg in differend places so "
      "this path is used to change the -I flags."
  )

  mark_as_advanced(${cache_var})

  if (${cache_var})
    include_directories("${${cache_var}}")
  endif()
endforeach()

if (is_debug)
  set(generated_debug EXTRA_DEBUG)
else()
  set(generated_debug)
endif()

bbuild_parser(
  SOURCES_VAR parser_sources
  TOKENS_FILE_VAR tokens_file
  LEMON "config/grammar.lemon"
  FLEX  "config/lexer.l"
  CXX_FILES
  ${generated_debug}
)

if (is_debug)
  # Primary controller of debug messing about.  See the defines.hpp files.
  set(defs "NERVE_DEVELOPER=1")
else()
  set(defs)
endif()

bbuild_exe(
  TARGET
    nerved
  SOURCES
    main.cpp
    cli/parse.cpp
    cli/settings.cpp
    output/configure.cpp
    output/logging.cpp
    config/parse.cpp
    config/config_parser.cpp
    config/pipeline_configs.cpp
    config/parse_context.cpp
    config/semantic_checker.cpp
    config/dump_config_yaml.cpp
    config/dump_config_yaml.cpp
    pipeline/configure.cpp
    pipeline/job.cpp
    pipeline/section.cpp
    pipeline/pipeline_data.cpp
    pipeline/process_stage_sequence.cpp
    pipeline/observer_stage_sequence.cpp
    pipeline/input_stage_sequence.cpp
    pipeline/progressive_buffer.cpp
    player/run.cpp
    server/local_server.cpp
    server/session.cpp
    util/pooled.cpp
    stages/information.cpp
    stages/create.cpp
    stages/ffmpeg.cpp
    stages/sdl.cpp
    btrace/crash_detector.cpp
    btrace/backtrace.cpp
    btrace/demangle.cpp
    btrace/assert.cpp
  GENERATED_SOURCES
    ${parser_sources}
  CPPDEFS
    # Dependncy on this is handled by bbuild_parser.
    "TOKENS_FILE=\"${tokens_file}\""
    # TODO:
    #   This should be done using a macro location to the configure file rather
    #   than making a huge command-line.
    "NERVED_VERSION=\"${PROJECT_VERSION}\""
    ${defs}
  LIBS
    ${DL_LIB} ${BOOST_SYSTEM_LIB} ${BOOST_THREAD_LIB}
  POSSIBLE_VARS
    LEMON_EXE FLEX_EXE DL_LIB BOOST_SYSTEM_LIB BOOST_THREAD_LIB
    AVCODEC_H_PATH AVFORMAT_H_PATH
)

# Wine fails to load dlls if this isn't set.  Don't ask why :)
if (MINGW)
  # TODO:
  #   This might get fixed by bbuild's dll features?
  set_property(
    TARGET "nerved" APPEND PROPERTY LINK_FLAGS "-Wl,--enable-auto-import"
  )
endif()

# Make this debug-conditional.  It necessary for btace doing dll lookups.
set(backtraces 1)
if (GNUCXX AND backtraces)
  set_property(
    TARGET "nerved" APPEND PROPERTY LINK_FLAGS "-rdynamic"
  )
endif()

bdoc_doxygen(
  FORMATS html
  TARGET doxygen_nerved
  INPUT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  DOXYFILE "${CMAKE_SOURCE_DIR}/Doxyfile.default"
  NO_DEFAULT_INSTALL
)
