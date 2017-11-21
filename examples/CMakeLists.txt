include_directories(${CMAKE_BASE_DIR})

add_executable(serialization serialization.cpp)
target_link_libraries (serialization corecpp)

add_executable(command command.cpp)
target_link_libraries (command corecpp)
