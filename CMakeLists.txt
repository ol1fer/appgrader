cmake_minimum_required(VERSION 3.20)
project(AppGrader VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# No exceptions, no RTTI (C++ only — these flags are invalid for C)
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/EHs->
                        $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else()
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets DBus)
qt_standard_project_setup()

# Core engine — plain C, zero platform deps
set(CORE_SOURCES
    src/core/color_math.c
    src/core/lut3d.c
    src/core/cube_parser.c
    src/core/icc_writer.c
    src/core/spatial_params.c
)

# Platform backend
if(WIN32)
    set(BACKEND_SOURCES src/backend/windows/windows_backend.cpp)
elseif(UNIX)
    set(BACKEND_SOURCES src/backend/linux/linux_backend.cpp)
endif()

# Config and UI
set(APP_SOURCES
    src/main.cpp
    src/config/preset_manager.cpp
    src/config/settings.cpp
    src/ui/MainWindow.cpp
    src/ui/KWinInstaller.cpp
    src/ui/EffectSetupDialog.cpp
    src/ui/panels/BasicPanel.cpp
    src/ui/panels/ColorPanel.cpp
    src/ui/panels/ColorWheelsPanel.cpp
    src/ui/panels/HslPanel.cpp
    src/ui/panels/CurvesPanel.cpp
    src/ui/panels/LutPanel.cpp
    src/ui/panels/EffectsPanel.cpp
    src/ui/widgets/LabeledSlider.cpp
    src/ui/widgets/CurveEditor.cpp
    src/ui/widgets/ColorWheel.cpp
)

qt_add_executable(appgrader
    ${CORE_SOURCES}
    ${BACKEND_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(appgrader PRIVATE src)

target_link_libraries(appgrader PRIVATE Qt6::Widgets)

if(WIN32)
    target_link_libraries(appgrader PRIVATE mscms)
elseif(UNIX)
    target_link_libraries(appgrader PRIVATE Qt6::DBus)
endif()

install(TARGETS appgrader DESTINATION bin)

# KWin effect plugin — built only on Linux when KWin dev headers are present.
# Set -DAPPGRADER_REQUIRE_KWIN=ON in CI to fail loudly instead of skipping.
option(APPGRADER_REQUIRE_KWIN "Fail at configure time if KWin headers are missing" OFF)
if(UNIX AND NOT WIN32)
    find_package(KF6CoreAddons QUIET)
    find_package(KWin QUIET)
    if(KF6CoreAddons_FOUND AND KWin_FOUND)
        message(STATUS "KWin found — building appgrader_lut effect plugin")
        set(KDE_INSTALL_USE_QT_SYS_PATHS ON)
        include(KDEInstallDirs6 OPTIONAL)
        add_subdirectory(src/kwin_effect)
    elseif(APPGRADER_REQUIRE_KWIN)
        message(FATAL_ERROR
            "KWin dev headers not found. KF6CoreAddons_FOUND=${KF6CoreAddons_FOUND} "
            "KWin_FOUND=${KWin_FOUND}. Install kwin-dev / kcoreaddons-dev or unset "
            "APPGRADER_REQUIRE_KWIN.")
    else()
        message(STATUS "KWin dev headers not found — skipping effect plugin")
    endif()
endif()
