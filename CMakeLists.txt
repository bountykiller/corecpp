cmake_minimum_required(VERSION 3.8)

project(corecpp)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CORECPP_VERSION "0.6")
set(PKGCONFIG_DIR ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
set(CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/corecpp)
set(PKGCONFIG_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp.pc)
set(CMAKE_FILES ${CMAKE_CURRENT_BINARY_DIR}/corecpp-config.cmake)

# options
option(BUILD_EXAMPLES "Build examples" ON)
option(RUN_TESTS "Run Test" ON)

add_subdirectory(include)
add_subdirectory(src)

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif(BUILD_EXAMPLES)

if(RUN_TESTS)
	enable_testing ()
	add_subdirectory(tests)
endif(RUN_TESTS)

configure_file("corecpp.pc.in" "corecpp.pc" @ONLY)
configure_file("corecpp-config.cmake.in" "corecpp-config.cmake" @ONLY)
install (FILES ${PKGCONFIG_FILES} DESTINATION ${PKGCONFIG_DIR})
install (FILES ${CMAKE_FILES} DESTINATION ${CMAKE_DIR})

