add_library(stb STATIC
    stb_image.c
    stb_image.h
    stb_truetype.c
    stb_truetype.h
    stb_vorbis.c
    stb_vorbis.h
)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
