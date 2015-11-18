project(corecpp)

cmake_minimum_required(VERSION 2.6)

SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CORECPP_VERSION "0.1")
SET(PKGCONFIG_DIR ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
SET(CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/corecpp)
SET(PKGCONFIG_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp.pc)
SET(CMAKE_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp-config.cmake)

add_subdirectory(src)
add_subdirectory(include)
configure_file("corecpp.pc.in" "corecpp.pc" @ONLY)
configure_file("corecpp-config.cmake.in" "corecpp-config.cmake" @ONLY)
install (FILES ${PKGCONFIG_FILES} DESTINATION ${PKGCONFIG_DIR})
install (FILES ${CMAKE_FILES} DESTINATION ${CMAKE_DIR})

