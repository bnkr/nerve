##############
## Prologue ##
##############
cmake_minimum_required(VERSION 2.6)
# For ease of use later
set(PROJECT_NAME nerve)
set(PROJECT_VERSION "0.0.0")
project(${PROJECT_NAME})

set(BCMAKE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/build-aux/bcmake/include")
set(BCMAKE_SCRIPT_DIR "${CMAKE_SOURCE_DIR}/build-aux/bcmake/aux")
include("${BCMAKE_INCLUDE_DIR}/bcmake.cmake")
include("${BCMAKE_INCLUDE_DIR}/bdoc.cmake")
include("${BCMAKE_INCLUDE_DIR}/doxygen.cmake")

bsetup_init(NO_OBSOLETE_OPTIONS)

#######################
## Check for headers ##
#######################

include(CheckFunctionExists)

find_file(HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H   "ffmpeg/libavcodec/avcodec.h")
find_file(HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H "ffmpeg/libavformat/avformat.h")
find_file(HAVE_FFMPEG_AVCODEC_H  "ffmpeg/avcodec.h")
find_file(HAVE_FFMPEG_AVFORMAT_H "ffmpeg/avformat.h")
find_file(HAVE_SDL_H "SDL.h")

mark_as_advanced(
  HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
  HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
  HAVE_FFMPEG_AVCODEC_H
  HAVE_FFMPEG_AVFORMAT_H
  HAVE_SDL_H
)

####################
## Check for libs ##
####################

find_library(BOOST_FS_LIB "boost_filesystem-mt")
if (NOT BOOST_FS_LIB)
  find_library(BOOST_FS_LIB "boost_filesystem")
  if (BOOST_FS_LIB)
    message("Warning: linking to boost_filesystem which doesn't have the -mt")
    message("         suffix could mean that it is not thread-safe.  If there")
    message("         is no existing -mt library, then it is probably OK.")
  endif()
endif()
find_library(BOOST_PROGOPT_LIB "boost_program_options-mt")
if (NOT BOOST_PROGOPT_LIB)
  find_library(BOOST_PROGOPT_LIB "boost_program_options")
endif()
find_library(BOOST_THREAD_LIB "boost_thread-mt")
if (NOT BOOST_THREAD_LIB)
  find_library(BOOST_THREAD_LIB "boost_thread")
endif()
# needed on windows so don't remove it!
find_library(BOOST_SYSTEM_LIB "boost_system-mt")
if (NOT BOOST_SYSTEM_LIB)
  find_library(BOOST_SYSTEM_LIB "boost_system")
endif()

find_library(AVUTIL_LIB   "avutil")
find_library(AVFORMAT_LIB "avformat")
find_library(AVCODEC_LIB  "avcodec")

mark_as_advanced(
  BOOST_FS_LIB BOOST_PROGOPT_LIB BOOST_THREAD_LIB BOOST_SYSTEM_LIB
  AVUTIL_LIB AVFORMAT_LIB AVCODEC_LIB
)

# FindSDL doesn't work properly with crosscomp due to the use of sdl-config
# binary.
if (NOT CMAKE_CROSS_COMPILING)
  # defines SDL_LIBRARY and all the stuff from sdl-config bin.
  find_package(SDL REQUIRED)
else()
  message("Warning: SDL library can't be auto-detected when cross-compiling.  You may need")
  message("         to add additional cflags and include directories.")
  find_library(SDL_LIBRARY "SDL")
endif()

#############################################
## Configure things based on what we found ##
#############################################

# Guess ffmpeg's include path which is normally includeroot/ffmpeg.
if (HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H)
  get_filename_component(p ${HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H} PATH)
  get_filename_component(ff_inc_path ${p} PATH)
else()
  get_filename_component(ff_inc_path ${HAVE_FFMPEG_AVFORMAT_H} PATH)
endif()
include_directories("${ff_inc_path}")

# config.hpps
set(NERVED_CONFIG_HPP_OUTPUT "${CMAKE_BINARY_DIR}/include/nerved_config.hpp")
configure_file("${CMAKE_SOURCE_DIR}/build-aux/nerved_config.hpp.cmake" "${NERVED_CONFIG_HPP_OUTPUT}")

set(NERVE_VERSION ${PROJECT_VERSION})
set(NERVE_HOST_SYSTEM ${CMAKE_HOST_SYSTEM})
set(NERVE_TARGET_SYSTEM ${CMAKE_SYSTEM})
set(NERVE_CONFIG_HPP_OUTPUT "${CMAKE_BINARY_DIR}/include/nerve_config.hpp")
configure_file("${CMAKE_SOURCE_DIR}/build-aux/nerve_config.hpp.cmake" "${NERVE_CONFIG_HPP_OUTPUT}")

###########################
## Building normal stuff ##
###########################

set(BIN_NERVED "nerved")
set(BIN_NERVE "nerve")

set(NERVED_SRCS "src/nerved/main.cpp")
set(NERVE_SRCS  "src/nerve/main.cpp" "src/nerve/settings.cpp")

add_executable(${BIN_NERVED} ${NERVED_SRCS})
add_executable(${BIN_NERVE}  ${NERVE_SRCS})
target_link_libraries(${BIN_NERVE} ${BOOST_PROGOPT_LIB})
target_link_libraries(${BIN_NERVED} ${BOOST_PROGOPT_LIB})
# Wine fails to load dlls if this isn't set.  Don't ask why :)
if (MINGW)
  set_property(
    TARGET ${BIN_NERVED}
           ${BIN_NERVE}
    APPEND
    PROPERTY LINK_FLAGS
             "-Wl,--enable-auto-import"
  )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
  set(RES_DIR "src/research")

  set(FFMPEG_LIBS "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}")

  set(ALL_IO_LIBS
    ${SDL_LIBRARY}
    ${FFMPEG_LIBS}
  )

  set(UTILITY_LIBS
    "${BOOST_FS_LIB}" "${BOOST_THREAD_LIB}"
  )

  if (WIN32)
    list(APPEND UTILITY_LIBS "${BOOST_SYSTEM_LIB}")
  endif()

  # gapless-playback
  set(bin_name "gapless-playback")
  butil_add_executable(
    TARGET "${bin_name}"
    SOURCES
      "${RES_DIR}/${bin_name}/main.cpp"
      "${RES_DIR}/${bin_name}/play.cpp"
      "${RES_DIR}/${bin_name}/output.cpp"
      "${RES_DIR}/${bin_name}/chunkinate.cpp"
      "${RES_DIR}/${bin_name}/shared_data.cpp"
      "${RES_DIR}/${bin_name}/dump_file.cpp"
      "${RES_DIR}/${bin_name}/degapifier.cpp"
    LIBS ${ALL_IO_LIBS} ${UTILITY_LIBS}
  )

  # tripple-gapless-playback
  set(bin_name "tripple-gapless-playback")
  set(src "${RES_DIR}/tripple-gapless-playback.cpp")
  add_executable("${bin_name}" ${src})
  target_link_libraries("${bin_name}" ${ALL_IO_LIBS} ${UTILITY_LIBS})

  # interrupted-playback
  set(bin_name "interrupted-playback")
  set(src "src/research/interrupted-playback.cpp")
  add_executable("${bin_name}" ${src})
  target_link_libraries("${bin_name}" ${ALL_IO_LIBS} ${UTILITY_LIBS})
endif()

# Do tests
add_subdirectory("test")

############################
## Building Documentation ##
############################

add_doxygen_directives(
  TARGET doxygen
  # Note: later I will need to document the interface lib.
  INPUTS "src/"
  ARGS_VAR ad_args
  DOCS_MIRROR "${CMAKE_SOURCE_DIR}/doc"
  DEFAULT_DOXYFILE "${CMAKE_SOURCE_DIR}/Doxyfile.default"
  INSTALL ${WANT_DOCS}
)
add_doxygen(${ad_args})

##################
## Installation ##
##################

bsetup_auto_install(
  RUNNABLES_VAR runnables
  RUNNABLES ${BIN_NERVED} "Nerve Daemon"
            ${BIN_NERVE}  "Nerve CLI Client"
  BINARIES_VAR stuff_to_strip
  TARGETS ${BIN_NERVED} ${BIN_NERVE}
  # INCLUDE_DIRS dir...
  # [HEADER_EXCLUDE pattern... | NO_HEADERS]
  # WINDOWS_LIBS library...]
  # AUX_DLLS
)

#################
## CPack Stuff ##
#################

bsetup_cpack(
  TITLE       "Nerve"
  LONG_DESCRIPTION "A fast and simple client/server audio player."
  VENDOR      "bunker"
  EMAIL       "bunkerprivate@googlemail.com"
  URL         "http://www.bunkerprivate.com/"
  BINARIES    ${stuff_to_strip}
  RUNNABLES   ${runnables}
)

