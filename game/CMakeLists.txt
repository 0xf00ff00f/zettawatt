if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    find_package(OpenGL REQUIRED)
    find_package(SDL REQUIRED)
    find_package(GLEW REQUIRED)
endif()

set(gx_SOURCES
    fontcache.cpp
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
    glm
    stb
    spdlog
)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_link_libraries(gx
    PUBLIC
        GLEW::GLEW
        OpenGL::GL
        SDL::SDL
    )
endif()

# add_executable(tst_textrendering tst_textrendering.cpp)
# target_link_libraries(tst_textrendering gx)
# 
# add_executable(tst_textbox
#     tst_textbox.cpp
#     uipainter.cpp
# )
# target_link_libraries(tst_textbox gx)

set(game_SOURCES
    main.cpp
    world.cpp
    world.h
    uipainter.cpp
    uipainter.h
    techgraph.cpp
    techgraph.h
    theme.cpp
    theme.h
    gamewindow.cpp
    gamewindow.h
)

add_executable(game
    ${game_SOURCES}
)

target_link_libraries(game
    gx
    fmt
    rapidjson
) 

target_compile_definitions(game PUBLIC GLM_FORCE_SWIZZLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(game PROPERTIES LINK_FLAGS "-s FULL_ES3=1 --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets")
endif()

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    add_custom_command(TARGET game
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    )
endif()
