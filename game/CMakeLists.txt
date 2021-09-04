find_package(OpenGL REQUIRED)

set(gx_SOURCES
    fontcache.cpp
    glwindow.cpp
    ioutil.cpp
    lazytexture.cpp
    pixmap.cpp
    shaderprogram.cpp
    spritebatcher.cpp
    textureatlas.cpp
    textureatlaspage.cpp
    texture.cpp
    loadprogram.cpp
    shadermanager.cpp
    fontcache.h
    glwindow.h
    ioutil.h
    lazytexture.h
    pixmap.h
    shaderprogram.h
    spritebatcher.h
    textureatlas.h
    textureatlaspage.h
    texture.h
    loadprogram.h
    shadermanager.h
)

add_library(gx
    ${gx_SOURCES}
)

target_include_directories(gx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gx
PUBLIC
    OpenGL::GL
    glfw
    glm
    stb
    spdlog
    glew
)

add_executable(tst_textrendering tst_textrendering.cpp)
target_link_libraries(tst_textrendering gx)

add_executable(tst_textbox
    tst_textbox.cpp
    uipainter.cpp
)
target_link_libraries(tst_textbox gx)

set(game_SOURCES
    main.cpp
    world.cpp
    world.h
    uipainter.cpp
    uipainter.h
    techgraph.cpp
    techgraph.h
)
add_executable(game
    ${game_SOURCES}
)
target_link_libraries(game
    gx
    fmt
) 

if (NOT WIN32)
    add_custom_command(TARGET game
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    )
endif()
