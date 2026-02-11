cmake_minimum_required(VERSION 3.20)
project(x16unifontedit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(x16unifontedit
    src/main.cpp
    src/MainWindow.cpp
    src/UlfFont.cpp
    src/GlyphEditor.cpp
    src/GlyphGrid.cpp
    src/CompositePreview.cpp
    src/UnicodeMapEditor.cpp
    src/UndoCommands.cpp
    src/ColorSettings.cpp
    src/UnicodeNames.cpp
)

target_link_libraries(x16unifontedit PRIVATE Qt6::Widgets)

# ICU for Unicode character names
find_library(ICU_LIBRARY icucore)
if(ICU_LIBRARY)
    target_link_libraries(x16unifontedit PRIVATE ${ICU_LIBRARY})
endif()

# macOS frameworks for font fallback
if(APPLE)
    target_link_libraries(x16unifontedit PRIVATE
        "-framework CoreText"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
    )
endif()
