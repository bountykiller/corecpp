include_directories("../include/")
add_executable(test_serialization test_serialization.cpp)
target_link_libraries (test_serialization corecpp)

add_executable(test_command test_command.cpp)
target_link_libraries (test_command corecpp)

add_executable(test_reflection test_reflection.cpp)
target_link_libraries (test_reflection corecpp)

add_test(NAME "test_serialization" COMMAND test_serialization)
add_test(NAME "test_command"       COMMAND test_command)
add_test(NAME "test_reflection"    COMMAND test_reflection)
