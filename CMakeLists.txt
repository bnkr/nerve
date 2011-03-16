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
## Finding Libraries ##
#######################

binfo_find_lib(
  NAMES boost_filesystem-mt boost_filesystem
  VAR BOOST_FILESYSTEM_LIB
  VAR_DOC "The boost.filesystem library."
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

###########################
## Building normal stuff ##
###########################

# Building all the nerve programs is a bit on the complicated side so my usual
# convention of keeping it all in one CMakeLists.txt is not used.

add_subdirectory("src/nerved")
add_subdirectory("src")
add_subdirectory("test")

############################
## Building Documentation ##
############################

bdoc_asciidoc(
  TARGET manpage_nerve
  FORMATS manpage xhtml
  INPUTS doc/nerve.1.txt
)

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

