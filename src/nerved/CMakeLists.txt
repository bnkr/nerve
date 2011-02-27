# Copyright (C) 2011, James Webber.
# Distributed under a 3-clause BSD license.  See COPYING.

# TODO:
#   This lot should be moved into the main directory.

bbuild_parser(
  SOURCES_VAR parser_sources
  TOKENS_FILE_VAR tokens_file
  LEMON "config/grammar.lemon"
  FLEX  "config/lexer.l"
  CXX_FILES
  EXTRA_DEBUG
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(defs "NERVE_LEXER_TRACE=1" "NERVE_PARSER_TRACE=1")
else()
  set(defs)
endif()

binfo_find_lib(
  VAR DL_LIB
  VAR_DOC "Interface to the dynamic linker"
  TASK nerved
  TASK_DOC "Used under debug configurations to get nicer backtraces."
  NAME dl
)

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
    config/dump_config_yaml.cpp
    config/dump_config_yaml.cpp
    pipeline/configure.cpp
    player/run.cpp
    util/pooled.cpp
    btrace/crash_detector.cpp
    btrace/backtrace.cpp
    btrace/demangle.cpp
  GENERATED_SOURCES
    ${parser_sources}
  CPPDEFS
    # Dependncy on this is handled by bbuild_parser.
    "TOKENS_FILE=\"${tokens_file}\""
    "NERVED_VERSION=\"${PROJECT_VERSION}\""
    ${defs}
  LIBS
    ${DL_LIB}
  POSSIBLE_VARS
    LEMON_EXE FLEX_EXE DL_LIB
)

# Wine fails to load dlls if this isn't set.  Don't ask why :)
if (MINGW)
  # TODO:
  #   This might get fixed by bbuild's dll features?
  set_property(
    TARGET "nerved" APPEND PROPERTY LINK_FLAGS "-Wl,--enable-auto-import"
  )
endif()

# Make this debug-conditional.
set(backtraces 1)
if (GNUCXX AND backtraces)
  set_property(
    TARGET "nerved" APPEND PROPERTY LINK_FLAGS "-rdynamic"
  )
endif()
