bbuild_exe(
  NO_INSTALL
  TARGET sock-server
  SOURCES server.cpp
  LIBS ${BOOST_SYSTEM_LIB}
)

bbuild_exe(
  NO_INSTALL
  TARGET sock-client
  LIBS "${BOOST_SYSTEM_LIB}"
  SOURCES client.cpp
)

