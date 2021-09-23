add_library(common
        application.hpp
        application.cpp
        shader.hpp
        shader.cpp
        mesh.hpp
        mesh.cpp
        data.hpp
        data.cpp
        texture.hpp
        texture.cpp
        gltf.hpp
        gltf.cpp
        framebuffer.hpp
        framebuffer.cpp
        renderer.hpp
        renderer.cpp
        utils.hpp
        utils.cpp
        profile.h
        )

target_link_libraries(common PUBLIC
        glfw
        glew_s
        imgui
        glm
        tinygltf
        microprofile)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glew/include)
target_compile_features(common PUBLIC cxx_std_17)
configure_file(config.in.h ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
target_include_directories(common PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
