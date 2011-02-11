# Copyright (C) 2011, James Webber.
# Distributed under a 3-clause BSD license.  See COPYING.

bbuild_parser(
  SOURCES_VAR parser_sources
  LEMON grammar.lemon
  TOKENS_FILE_VAR tokens_file
  FLEX  lexer.l
  CXX_FILES
  EXTRA_DEBUG
)

# Kind of annoying but it means we can easily include source dir files from the
# lexer and parser which (ironically) it's nicer to do so that we can use quote
# includes and the dependencies work properly.  Unfortunately the compiler
# doesn't use #line info to find the current file because it can cause some
# issues when you dstribute build sournces.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(defs "NERVE_LEXER_TRACE=1" "NERVE_PARSER_TRACE=1")
else()
  set(defs)
endif()

bbuild_exe(
  TARGET config-proto
  SOURCES
    main.cpp
    config_parser.cpp
    parse_context.cpp
    pooled.cpp
  GENERATED_SOURCES
    ${parser_sources}
  CPPDEFS
    # Dependncy on this is handled by bbuild_parser.
    "TOKENS_FILE=\"${tokens_file}\""
    ${defs}
  NO_INSTALL
)
