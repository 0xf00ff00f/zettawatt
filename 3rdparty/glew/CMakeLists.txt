add_library(glew STATIC src/glew.c)
target_compile_definitions(glew PUBLIC GLEW_STATIC)
target_include_directories(glew PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
