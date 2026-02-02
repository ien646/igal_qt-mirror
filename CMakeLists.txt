cmake_minimum_required(VERSION 4.0)

set(PROJECT_NAME igal_qt)

include(cmake/CPM.cmake)

project(igal_qt VERSION 1.12.1)

if(WIN32)
    if(DEFINED ENV{QTDIR})
        set(CMAKE_PREFIX_PATH $ENV{QTDIR})
        add_definitions(-DUNICODE -D_UNICODE)
    endif()
endif()

CPMAddPackage(
    NAME libien
    GIT_REPOSITORY https://github.com/ien646/libien
    GIT_TAG master
)

find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)

find_package(OpenMP REQUIRED)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    "src/main.cpp"

    "src/CachedMediaProxy.hpp"
    "src/CachedMediaProxy.cpp"

    "src/ClickableSlider.hpp"
    "src/ClickableSlider.cpp"

    "src/HelpOverlay.hpp"
    "src/HelpOverlay.cpp"

    "src/InfoOverlayWidget.hpp"
    "src/InfoOverlayWidget.cpp"

    "src/ListSelectWidget.hpp"
    "src/ListSelectWidget.cpp"

    "src/MainWindow.hpp"
    "src/MainWindow.cpp"   

    "src/MediaWidget.hpp"
    "src/MediaWidget.cpp"

    "src/PreviewStrip.hpp"
    "src/PreviewStrip.cpp"

    "src/Utils.cpp"

    "src/VideoControls.hpp"
    "src/VideoControls.cpp"

    "src/VideoPlayerWidget.hpp"
    "src/VideoPlayerWidget.cpp"

    "rsc/fonts.qrc"
    "rsc/icons.qrc"
)

target_compile_definitions(${PROJECT_NAME} PUBLIC 
    "IGAL_QT_VERSION=\"${CMAKE_PROJECT_VERSION}\""
    "IGAL_QT_BUILD_MODE=\"${CMAKE_BUILD_TYPE}\""
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    AUTOMOC ON
    AUTORCC ON
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    libien 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Multimedia 
    Qt6::MultimediaWidgets
    ${OpenMP_CXX_LIB_NAMES})

install(TARGETS ${PROJECT_NAME}
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    qt_generate_deploy_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        NO_TRANSLATIONS
    )
else()
    qt_generate_deploy_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        NO_TRANSLATIONS
        NO_PLUGINS # Use system installed plugins (i.e. JXL support)
    )
endif()
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

if(WIN32 AND NOT MINGW)
    find_program(WINDEPLOYQT NAMES windeployqt.exe HINTS ${CMAKE_PREFIX_PATH} REQUIRED)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${WINDEPLOYQT} $<TARGET_FILE:${PROJECT_NAME}>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

include(cmake/CPackSettings.cmake)