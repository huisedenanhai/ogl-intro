add_executable(hello_world main.cpp)
target_compile_features(hello_world PRIVATE cxx_std_17)

target_link_libraries(hello_world PRIVATE glfw glew_s)
target_include_directories(hello_world PRIVATE ${CMAKE_SOURCE_DIR}/third_party/glew/include)