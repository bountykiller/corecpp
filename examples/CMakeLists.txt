add_executable(serialization serialization.cpp)
target_link_libraries (serialization corecpp)
target_include_directories(serialization PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(serialization PRIVATE "${CMAKE_SOURCE_DIR}/include")

add_executable(command command.cpp)
target_link_libraries (command corecpp)
target_include_directories(command PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(command PRIVATE "${CMAKE_SOURCE_DIR}/include")

add_executable(reflection reflection.cpp)
target_link_libraries (reflection corecpp)
target_include_directories(reflection PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(reflection PRIVATE "${CMAKE_SOURCE_DIR}/include")

add_executable(serialize_bench serialize_bench.cpp)
target_link_libraries (serialize_bench corecpp)
target_include_directories(serialize_bench PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(serialize_bench PRIVATE "${CMAKE_SOURCE_DIR}/include")
