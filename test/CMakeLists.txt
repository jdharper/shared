project (test CXX)

include_directories(../.. ..)
link_directories(C:\git\cpptest\build\Debug)
add_executable(test shared test_main)
target_link_libraries(test cpptest.lib)
