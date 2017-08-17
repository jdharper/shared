project (test CXX)

include_directories(../.. ..)
link_directories(../../../cpptest/build/Debug)
add_executable(shared_test shared test_main)
target_link_libraries(shared_test cpptest.lib)
