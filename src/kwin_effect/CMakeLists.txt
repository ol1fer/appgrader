find_package(KF6CoreAddons REQUIRED)
find_package(KWin REQUIRED)

# KWin effect plugins must be MODULE libraries (dlopen-loaded, not linked)
add_library(appgrader_lut MODULE
    AppGraderEffect.cpp
    "${CMAKE_SOURCE_DIR}/src/core/spatial_params.c"
)

# Pick up shared core headers (SpatialParams struct + spatial_params_init proto).
target_include_directories(appgrader_lut PRIVATE "${CMAKE_SOURCE_DIR}/src")

set_target_properties(appgrader_lut PROPERTIES
    AUTOMOC ON
    CXX_STANDARD 20
    PREFIX ""
)

target_compile_definitions(appgrader_lut PRIVATE
    KPLUGINFACTORY_PLUGIN_CLASS_INTERNAL_NAME=appgrader_lut_factory
)

# KWin's plugin loader uses dynamic_cast to verify the factory type, so RTTI and
# exceptions must be enabled even though the rest of the project disables them.
target_compile_options(appgrader_lut PRIVATE -frtti -fexceptions)

target_link_libraries(appgrader_lut PRIVATE
    KF6::CoreAddons
    KWin::kwin
    Qt6::Core
)

get_target_property(_qt6_qmake Qt6::qmake IMPORTED_LOCATION)
execute_process(
    COMMAND "${_qt6_qmake}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE _qt6_plugin_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
install(TARGETS appgrader_lut
    LIBRARY DESTINATION "${_qt6_plugin_dir}/kwin/effects/plugins"
)

# Also install a copy alongside the application data so the running app can find
# a source .so to deploy to a user-local plugin path (avoids needing sudo to set
# up the effect after installation). The Setup dialog searches this location.
include(GNUInstallDirs)
install(TARGETS appgrader_lut
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/appgrader"
)
