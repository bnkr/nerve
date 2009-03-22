##############
## Prologue ##
##############
cmake_minimum_required(VERSION 2.6)
# For ease of use later
set(PROJECT_NAME nerve)
set(PROJECT_VERSION "0.0")
project(${PROJECT_NAME})

string(REGEX REPLACE "^([0-9]+).*" "\\1" PROJECT_MAJOR ${PROJECT_VERSION})
string(REGEX REPLACE "^[^.]+\\.([0-9]+).*" "\\1" PROJECT_MINOR ${PROJECT_VERSION})
string(REGEX REPLACE "^[^.]+\\.[^.]+\\.([0-9]+).*" "\\1" PROJECT_PATCH ${PROJECT_VERSION})
string(REGEX REPLACE "^[^.]+\\.[^.]+\\.[^\\-]-([0-9]+).*" "\\1" PROJECT_SNAPSHOT ${PROJECT_VERSION})

include("build-aux/doxygen.cmake")
include("build-aux/manpage.cmake")

# General properties of the build
include_directories("${CMAKE_SOURCE_DIR}/include/" "${CMAKE_BINARY_DIR}/include/")

enable_testing()

# For different types of CMAKE_BUILD_TYPE, add -Wall.
if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall" STRING CACHE FORCE "CXX compiler parmeters when BUILD_TYPE is Debug")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall" STRING CACHE FORCE "CXX compiler parmeters when BUILD_TYPE is Debug")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -Wall" STRING CACHE FORCE "CXX compiler parmeters when BUILD_TYPE is Debug")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall" STRING CACHE FORCE "C compiler parmeters when BUILD_TYPE is Debug")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall" STRING CACHE FORCE "C compiler parmeters when BUILD_TYPE is Release")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -Wall" STRING CACHE FORCE "C compiler parmeters when BUILD_TYPE is RelWithDebInfo")
endif()

# Global consts for libraries etc.

# General options
option(WANT_DOCS "Install documentation - when off, all other docs options are off." YES)
option(WANT_DOXYGEN_HTML "Install HTML made by doxygen" NO)
option(WANT_DOXYGEN_PDF "Install PDF from doxygen (needs pdfLaTeX)" NO)
option(WANT_DOCS_MAN "Install manpages if there are any." YES)
option(WANT_DOXYGEN_REBUILD "Regenerate doxygen and install that instead of the distributed docs." NO)

set(DOXYGEN_DOXYFILE_TEMPLATE "${CMAKE_SOURCE_DIR}/Doxyfile.default" CACHE STRING 
    "Base doxyfile.  Mostly useful for developers.  Certain vars will be overwritten by cmake (in/output paths mostly).")

mark_as_advanced(
  DOXYGEN_DOXYFILE_TEMPLATE 
  WANT_DOXYGEN_REBUILD
  WANT_DOXYGEN_HTML 
  WANT_DOXYGEN_PDF
  WANT_DOCS_MAN
)

###################################
## Checking for required headers ##
###################################

include(CheckFunctionExists) 

find_file(HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H   "ffmpeg/libavcodec/avcodec.h")
find_file(HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H "ffmpeg/libavformat/avformat.h")
find_file(HAVE_FFMPEG_AVCODEC_H  "ffmpeg/avcodec.h")
find_file(HAVE_FFMPEG_AVFORMAT_H "ffmpeg/avformat.h")

# Note: use booleans like bdbg.
set(CONFIG_HPP_OUTPUT "${CMAKE_BINARY_DIR}/include/nerved_config.hpp")
configure_file("${CMAKE_SOURCE_DIR}/build-aux/nerved_config.hpp.cmake" "${CONFIG_HPP_OUTPUT}")

###########################
## Building normal stuff ##
###########################

# This one is a bit tricky.  Some versions of ffmpeg have ffmpeg/libavcodec/file.h and 
# others have ffmpeg/file.h.  The docs use ffmpeg/file.h, but the latest versions of the
# library doesn't agree.  Also, the headers themselves include "libavcodec/file.h".  
# Therefore I hedge my bets using configure_file() and also setting the root directory 
# of ffmpeg includable.  Hopefully it doesn't break anything, but if you're reading this 
# it probably did so good luck fixing it!
if (UNIX)
  include_directories("/usr/include/ffmpeg/")
endif()

# Example follows:
# add_library(${LIB_BDBG_TRACE_A} STATIC "src/lib_bdbg_trace.cpp")
# set_target_properties(${LIB_BDBG_TRACE_A} PROPERTIES OUTPUT_NAME "bdbg_trace")
# add_library(${LIB_BDBG_TRACE_S} SHARED "src/lib_bdbg_trace.cpp")
# set_target_properties(${LIB_BDBG_TRACE_S} PROPERTIES OUTPUT_NAME "bdbg_trace")
# set_target_properties(${LIB_BDBG_TRACE_S} PROPERTIES SOVERSION "${PROJECT_MAJOR}")
# set_target_properties(${LIB_BDBG_TRACE_S} PROPERTIES VERSION "${PROJECT_VERSION}")
# target_link_libraries(${LIB_BDBG_TRACE_S} "-lboost_thread")

find_library(BOOST_FS_LIB "boost_filesystem-mt")
if (NOT BOOST_FS_LIB)
  find_library(BOOST_FS_LIB "boost_filesystem")
  if (BOOST_FS_LIB)
    message("Warning: linking to boost_filesystem which doesn't have the -mt")
    message("         suffix could mean that it is not thread-safe.  If there")
    message("         is no other library to link to then it is probably OK.")
  endif()
endif()
find_library(AVUTIL_LIB   "avutil")
find_library(AVFORMAT_LIB "avformat")
find_library(AVCODEC_LIB  "avcodec")

set(BIN_NERVED "nerved")
set(BIN_NERVE "nerve")

set(NERVED_SRCS "src/nerved/main.cpp")
set(NERVE_SRCS  "src/nerve/main.cpp")

add_executable(${BIN_NERVED} ${NERVED_SRCS})
add_executable(${BIN_NERVE}  ${NERVE_SRCS})

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
  set(RES_DIR "src/research")

  # gapless-playback
  set(bin_name "gapless-playback")
  set(src "${RES_DIR}/gapless-playback.cpp")
  add_executable("${bin_name}" "${src}")
  target_link_libraries("${bin_name}" 
    "${BOOST_FS_LIB}" "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}"
  )

  # tripple-gapless-playback
  set(bin_name "tripple-gapless-playback")
  set(src "${RES_DIR}/tripple-gapless-playback.cpp")
  add_executable("${bin_name}" "${src}")
  target_link_libraries("${bin_name}" 
    "${BOOST_FS_LIB}" "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}"
  )

  # simple-media-player
  set(bin_name  "simple-media-player")
  set(src "${RES_DIR}/${bin_name}/main.cpp" "${RES_DIR}/${bin_name}/play.cpp")
  add_executable(${bin_name} ${src})
  target_link_libraries("${bin_name}" 
    "${BOOST_FS_LIB}" "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}"
  )

  # interrupted-playback
  set(bin_name "interrupted-playback")
  set(src "src/research/interrupted-playback.cpp")
  add_executable("${bin_name}" ${src})
  target_link_libraries("${bin_name}" 
    "${BOOST_FS_LIB}" "${AVUTIL_LIB}" "${AVFORMAT_LIB}" "${AVCODEC_LIB}"
  )
endif()

# Do tests
add_subdirectory("test")

############################
## Building Documentation ##
############################

if (WANT_DOXYGEN_PDF)
  list(APPEND DOXYGEN_WANTS "pdf")
endif()
if (WANT_DOXYGEN_HTML)
  list(APPEND DOXYGEN_WANTS "html")
endif()

set(DOXYGEN_CMAKE_VERBOSE "yes")

set(DOXYGEN_TARGET "doxygen")
set(DOXYGEN_INPUT_PATHS "include/")
doxgyen_setup_flags(
  DOXYGEN_FLAGS 
  "${DOXYGEN_TARGET}"
  "${DOXYGEN_WANTS}"
)

add_doxygen(
  ${DOXYGEN_TARGET}
  ${DOXYGEN_DOXYFILE_TEMPLATE}
  "${DOXYGEN_FLAGS}"
)

##################
## Installation ##
##################

message(STATUS "Adding install targets.")

set(INCLUDEDIR "include")
set(BINDIR "bin")
set(DATADIR "share")
set(LIBDIR "lib")
set(DOCDIR "${DATADIR}/doc/${PROJECT_NAME}")
set(MANDIR "${DATADIR}/man")
set(HTMLDIR "${DATADIR}/doc/${PROJECT_NAME}")
set(PDFDIR "${DATADIR}/doc/${PROJECT_NAME}")
set(DVIDIR "${DATADIR}/doc/${PROJECT_NAME}")
set(PSDIR "${DATADIR}/doc/${PROJECT_NAME}")


if (WANT_DOCS) 
  # Documentation
  install(
    FILES README COPYING 
    DESTINATION ${DOCDIR}
  )

  if (WANT_DOXYGEN_REBUILD)
    set(INSTALL_FROM)
  else()
    set(INSTALL_FROM "${CMAKE_SOURCE_DIR}/doc/")
  endif()

  # Note: this requires that the variables set by doxygen_setup_flags are still valid!
  doxygen_install_targets(
    "${DOXYGEN_TARGET}"
    "${DOXYGEN_WANTS}" 
    "${DOCDIR}" 
    "${INSTALL_FROM}"
  )

  if (WANT_DOCS_MAN)
    set(SRCMAN "${CMAKE_SOURCE_DIR}/doc/man")
    add_and_install_manpages("${MANDIR}" "${SRCMAN}/nerve.1" "${SRCMAN}/nerved.1")
  endif()
endif(WANT_DOCS)

# Header files
# install(
#   DIRECTORY "include/"
#   DESTINATION "${INCLUDEDIR}/"
#   FILES_MATCHING
#   PATTERN "detail/config.hpp" EXCLUDE
#   PATTERN "*.hpp"
# )

# install(
#   FILES ${CONFIG_HPP_OUTPUT}
#   DESTINATION "${INCLUDEDIR}/para/detail/"
# )

# Binaries
install(
  TARGETS ${BIN_NERVE} ${BIN_NERVED}
  RUNTIME DESTINATION ${BINDIR}
  ARCHIVE DESTINATION ${LIBDIR}
  LIBRARY DESTINATION ${LIBDIR}
)

#################
## CPack Stuff ##
#################

message(STATUS "Doing CPack stuff.")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Stuff.")
set(CPACK_PACKAGE_VENDOR "bunker")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/build-aux/install-readme.txt")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

set(CPACK_SOURCE_TGZ "ON")
set(CPACK_SOURCE_TBZ2 "OFF")
set(CPACK_SOURCE_TZ "OFF")

set(CPACK_BINARY_STGZ "OFF")
set(CPACK_BINARY_TBZ2 "OFF")
set(CPACK_BINARY_TGZ "OFF")
set(CPACK_BINARY_TZ "OFF")

# Yes, you really need all that escape crap.
set(CPACK_SOURCE_IGNORE_FILES "/\\\\..*/;~$;.*\\\\.swp$;/${CMAKE_BINARY_DIR}/;Makefile")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")

# name, title, ...
# set(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME} ${PROJECT-VERSION}")
if(WIN32)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  # set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  # set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} My Famous Project")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
  set(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
  set(CPACK_NSIS_MODIFY_PATH ON)
else()
  # set(CPACK_STRIP_FILES "bin/MyExecutable")
  set(CPACK_SOURCE_STRIP_FILES "")
endif()

include(CPack)

