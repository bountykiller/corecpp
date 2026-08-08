SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

include_directories("../include/")
find_package(Threads REQUIRED)
add_library(
    corecpp
    STATIC
        appender.cpp
        command_line.cpp
        diagnostic_manager.cpp
        io.cpp
        json.cpp
        xml.cpp
)
target_link_libraries(corecpp PUBLIC Threads::Threads)
install(TARGETS corecpp DESTINATION ${LIBDIR})
