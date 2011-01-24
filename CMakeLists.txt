##############
## Prologue ##
##############

cmake_minimum_required(VERSION 2.8)

include("build-aux/bcmake/include/bdoc.cmake")
include("build-aux/bcmake/devel/bdoc_doxygen_redo.cmake")

bconfig_project(
  NAME    nerve
  VERSION 0.0.0
)

#######################
## Check for headers ##
#######################

# Unfortunately ffmpeg gets installed all over the place, so we have to check
# this stuff to get the right include directories.
#
# TODO:
#   Use bconfig.
find_path(
  AVCODEC_PATH
  NAMES "avcodec.h"
  PATHS "/usr/include/ffmpeg/libavcodec" "/usr/include/ffmpeg" "/usr/include/libavcodec"
)
find_path(
  AVFORMAT_PATH
  NAMES "avformat.h"
  PATHS "/usr/include/ffmpeg/libavformat" "/usr/include/ffmpeg" "/usr/include/libavformat"
)
mark_as_advanced(AVFORMAT_PATH AVCODEC_PATH)

####################
## Check for libs ##
####################

binfo_find_lib(
  NAMES boost_filesystem-mt boost_filesystem
  VAR BOOST_FILESYSTEM_LIB
  VAR_DOC "The boost.filesystem library."
)

binfo_find_lib(
  NAMES boost_thread-mt boost_thread
  VAR BOOST_THREAD_LIB
  VAR_DOC "The boost.thread library."
)

binfo_find_lib(
  NAMES boost_system-mt boost_system
  VAR BOOST_SYSTEM_LIB
  VAR_DOC "The boost.system error handling library."
)

binfo_find_lib(
  NAMES boost_program_options-mt boost_program_options
  VAR BOOST_PROGRAM_OPTIONS_LIB
  VAR_DOC "The boost.program options commandline parser library."
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

###########################
## Building normal stuff ##
###########################

add_subdirectory("src")
add_subdirectory("test")

############################
## Building Documentation ##
############################

# add_doxygen_directives(
#   TARGET doxygen
#   INPUTS "src/"
#   ARGS_VAR ad_args
#   DOCS_MIRROR "${CMAKE_SOURCE_DIR}/doc"
#   DEFAULT_DOXYFILE "${CMAKE_SOURCE_DIR}/Doxyfile.default"
#   INSTALL ${WANT_DOCS}
# )
# add_doxygen(${ad_args})

##################
## Packaging ##
##################

bpackage_init()
bpackage_install()

bsetup_cpack(
  TITLE       "Nerve"
  LONG_DESCRIPTION "A fast and simple client/server audio player."
  VENDOR      "bunker"
  EMAIL       "bunkerprivate@googlemail.com"
  URL         "http://www.bunkerprivate.com/project/nerve"
)

