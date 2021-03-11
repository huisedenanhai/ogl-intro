add_executable(pbr main.cpp material.hpp material.cpp)

target_link_libraries(pbr PRIVATE common)