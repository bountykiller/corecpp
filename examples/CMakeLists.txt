add_executable(serialization serialization.cpp)
target_link_libraries (serialization corecpp)
target_include_directories(serialization PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(serialization PRIVATE "${CMAKE_SOURCE_DIR}/include")

add_executable(command command.cpp)
target_link_libraries (command corecpp)
target_include_directories(command PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(command PRIVATE "${CMAKE_SOURCE_DIR}/include")
