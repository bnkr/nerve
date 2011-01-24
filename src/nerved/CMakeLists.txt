set(NERVED_CONFIG_HPP_OUTPUT "${CMAKE_BINARY_DIR}/include/nerved_config.hpp")
configure_file("${CMAKE_SOURCE_DIR}/build-aux/nerved_config.hpp.cmake" "${NERVED_CONFIG_HPP_OUTPUT}")

bbuild_exe(
  TARGET nerved
  SOURCES
    main.cpp
  LIBS ${BOOST_PROGRAM_OPTIONS_LIB} ${BOOST_SYSTEM_LIB}
)

# Wine fails to load dlls if this isn't set.  Don't ask why :)
if (MINGW)
  # TODO:
  #   This might get fixed by bbuild's dll features?
  set_property(
    TARGET "nerved" APPEND PROPERTY LINK_FLAGS "-Wl,--enable-auto-import"
  )
endif()
