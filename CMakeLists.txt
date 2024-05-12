cmake_minimum_required(VERSION 3.25)

include(cmake/CPM.cmake)

project(igal_qt)

CPMAddPackage(
    NAME libien
    GIT_REPOSITORY https://github.com/ien646/libien
    GIT_TAG master
)

find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)

qt_add_executable(igal_qt
    "src/main.cpp"

    "src/CachedMediaProxy.cpp"
    "src/InfoOverlayWidget.cpp"
    "src/MainWindow.cpp"   
    "src/MediaWidget.cpp"
    "src/Utils.cpp"
    "src/VideoControls.cpp"
    "src/VideoPlayerWidget.cpp"

    "rsc/fonts.qrc"
    "rsc/icons.qrc"
)

set_target_properties(igal_qt PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(igal_qt PUBLIC libien Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)