cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME igal_qt)

include(cmake/CPM.cmake)

project(igal_qt VERSION 1.0.3)

CPMAddPackage(
    NAME libien
    GIT_REPOSITORY https://github.com/ien646/libien
    GIT_TAG master
)

find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)

qt_add_executable(${PROJECT_NAME}
    "src/main.cpp"

    "src/CachedMediaProxy.cpp"
    "src/InfoOverlayWidget.cpp"
    "src/ListSelectWidget.cpp"
    "src/MainWindow.cpp"   
    "src/MediaWidget.cpp"
    "src/Utils.cpp"
    "src/VideoControls.cpp"
    "src/VideoPlayerWidget.cpp"

    "rsc/fonts.qrc"
    "rsc/icons.qrc"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(${PROJECT_NAME} PUBLIC libien Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)

install(TARGETS ${PROJECT_NAME}
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})

if(LINUX)
    install(
        FILES 
            "${CMAKE_SOURCE_DIR}/install/linux/install.sh"
            "${CMAKE_SOURCE_DIR}/install/linux/igal_qt.desktop"
        DESTINATION .
        PERMISSIONS WORLD_READ WORLD_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )

    install(
        FILES "${CMAKE_SOURCE_DIR}/rsc/igal_qt.png"
        DESTINATION "rsc"
    )
endif()

include(cmake/CPackSettings.cmake)