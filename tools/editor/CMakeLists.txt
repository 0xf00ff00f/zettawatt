find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(editor_SOURCES
	main.cpp
	editorwindow.cpp
	editorwindow.h
	techgraphview.cpp
	techgraphview.h
    techgraph.cpp
    techgraph.h
    unitsettingswidget.cpp
    unitsettingswidget.h
)

add_executable(editor
    ${editor_SOURCES}
)

target_link_libraries(editor
    Qt5::Widgets
)
