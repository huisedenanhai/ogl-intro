add_library(tinygltf 
    tiny_gltf_impl.cpp 
    include/tiny_gltf.h 
    include/json.hpp
    include/stb_image_write.h 
    include/stb_image.h
)

target_compile_features(tinygltf PRIVATE cxx_std_17)

target_include_directories(tinygltf PUBLIC include)