set(NERVE_VERSION ${PROJECT_VERSION})
set(NERVE_HOST_SYSTEM ${CMAKE_HOST_SYSTEM})
set(NERVE_TARGET_SYSTEM ${CMAKE_SYSTEM})
set(NERVE_CONFIG_HPP_OUTPUT "${CMAKE_BINARY_DIR}/include/nerve_config.hpp")
configure_file("${CMAKE_SOURCE_DIR}/build-aux/nerve_config.hpp.cmake" "${NERVE_CONFIG_HPP_OUTPUT}")

bbuild_exe(
  TARGET nerve
  SOURCES
    main.cpp
    settings.cpp
  LIBS ${BOOST_PROGRAM_OPTIONS_LIB} ${BOOST_SYSTEM_LIB}
)

# Wine fails to load dlls if this isn't set.  Don't ask why :)
if (MINGW)
  # TODO:
  #   This might get fixed by bbuild's dll features?
  set_property(
    TARGET "nerve" APPEND PROPERTY LINK_FLAGS "-Wl,--enable-auto-import"
  )
endif()
