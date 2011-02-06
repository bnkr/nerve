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

bbuild_exe(
  TARGET config-proto
  SOURCES
    main.cpp
  GENERATED_SOURCES
    ${parser_sources}
  CPPDEFS
    "TOKENS_FILE=\"${tokens_file}\""
    "LEXER_DEFS_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/flex_definitions.hpp\""
    "PARSER_DEFS_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/lemon_definitions.hpp\""
  NO_INSTALL
)
