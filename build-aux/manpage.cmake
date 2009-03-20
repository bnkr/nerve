# This file is a simple interface to manpage generation.
# 
# FUNCTION: add_manpage_gz(OUTPUT_VAR, INPUT_FILE):
#   Build a gzipped manpage.  Should be installed manually.  Will always be
#   built into ${MANPAGE_OUTPUT_DIR}.
#   
#   Example:
#     add_manpage_gz(PAGE "doc/something.1")
#
#     install(FILES ${PAGE} DESTINATION ${MANDIR})
#
# FUNCTION: add_and_install_manpages(MANDIR, PAGE1, [...]):
#   Uses add_manpage_gz to build manpages and adds install targets for them.
#   
#   The install directory is:
#
#     ${MANDIR}/man$N 
#
#   Where $N is the last letter  of the input filename.
#

find_program(GZIP_EXE "gzip")
mark_as_advanced(GZIP_EXE)

set(MANPAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/man-gz")

if (NOT IS_DIRECTORY "${MANPAGE_OUTPUT_DIR}")
  file(MAKE_DIRECTORY ${MANPAGE_OUTPUT_DIR})

  set_property(
    DIRECTORY APPEND
    PROPERTY "ADDITIONAL_MAKE_CLEAN_FILES"
    "${MANPAGE_OUTPUT_DIR}"
  )
endif()

macro(add_manpage_gz output_var input)
  get_filename_component(ampgz_basename ${input} NAME)
  set(ampgz_out "${MANPAGE_OUTPUT_DIR}/${ampgz_basename}.gz")
  set("${output_var}" "${ampgz_out}")

  message("${input} -> ${ampgz_out}")
  add_custom_command(
    OUTPUT "${ampgz_out}"
    DEPENDS "${input}"
    COMMAND "${GZIP_EXE}" -c "${input}" > "${ampgz_out}"
    VERBATIM
    COMMENT "Gzip manpage ${ampgz_basename}"
  )

  add_custom_target(manpages ALL DEPENDS "${ampgz_out}")
endmacro(add_manpage_gz)

function(add_and_install_manpages mandir)
  set(pages "${ARGV}")
  if (NOT pages)
    message(FATAL_ERROR "add_and_install_manpages(): at least one manpage is required.")
  endif()

  list(REMOVE_AT pages 0)
  foreach (arg ${pages})
    string(LENGTH "${arg}" len)
    math(EXPR begin "${len} - 1")
    string(SUBSTRING "${arg}" "${begin}" "1" last_char)
    add_manpage_gz(output ${arg})
    set(dest "${mandir}/man${last_char}")
    install(FILES ${output} DESTINATION ${dest})
  endforeach()
endfunction()

