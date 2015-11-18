SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

include_directories("../include/")
add_library(corecpp STATIC command_line.cpp)
install(TARGETS corecpp DESTINATION ${LIBDIR})
