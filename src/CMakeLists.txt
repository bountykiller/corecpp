SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

include_directories("../include/")
add_library(corecpp STATIC command_line.cpp diagnostic_manager.cpp appender.cpp json.cpp)
install(TARGETS corecpp DESTINATION ${LIBDIR})
