project(corecpp)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CORECPP_VERSION "0.3")
set(PKGCONFIG_DIR ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
set(CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/corecpp)
set(PKGCONFIG_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp.pc)
set(CMAKE_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp-config.cmake)

# options
option(BUILD_EXAMPLES "Build examples" ON)

add_subdirectory(include)
add_subdirectory(src)

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif(BUILD_EXAMPLES)

configure_file("corecpp.pc.in" "corecpp.pc" @ONLY)
configure_file("corecpp-config.cmake.in" "corecpp-config.cmake" @ONLY)
install (FILES ${PKGCONFIG_FILES} DESTINATION ${PKGCONFIG_DIR})
install (FILES ${CMAKE_FILES} DESTINATION ${CMAKE_DIR})

