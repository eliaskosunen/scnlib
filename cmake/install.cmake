if (NOT SCN_INSTALL)
    return()
endif()

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/scn)

set(targets "scn")
if(NOT SCN_USE_EXTERNAL_SIMDUTF)
    list(APPEND targets simdutf)
endif()
if(NOT SCN_USE_EXTERNAL_FAST_FLOAT)
    list(APPEND targets fast_float)
endif()

install(TARGETS ${targets}
        EXPORT scn-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT scnlib_Development
)

install(DIRECTORY
            include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT scnlib_Development
)

install(EXPORT scn-targets
        FILE
            scn-targets.cmake
        NAMESPACE
            scn::
        DESTINATION
            "${INSTALL_CONFIGDIR}"
        COMPONENT
            scnlib_Development
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/scn-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/scn-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/scn-config.cmake"
        INSTALL_DESTINATION "${INSTALL_CONFIGDIR}"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/scn-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/scn-config-version.cmake"
        DESTINATION "${INSTALL_CONFIGDIR}"
        COMPONENT scnlib_Development
)

export(EXPORT scn-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/scn-targets.cmake"
        NAMESPACE scn::
)

export(PACKAGE scn)
