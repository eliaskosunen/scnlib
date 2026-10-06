if (NOT SCN_INSTALL)
    return()
endif()

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/scn)

set(targets "scn")
if (SCN_FAST_FLOAT_BUNDLED)
    list(APPEND targets fast_float)
endif ()
if (SCN_MODULES)
    list(APPEND targets scn_module)
endif ()

set(install_file_set)
if (SCN_MODULES AND SCN_USE_CMAKE_MODULES)
    set(install_file_set FILE_SET scn DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/scn" COMPONENT scnlib_Development)
endif ()

install(TARGETS ${targets}
        EXPORT scn-targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT scnlib_Development
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT scnlib_Development
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT scnlib_Development
        ${install_file_set}
)

install(DIRECTORY
            include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT scnlib_Development
)

install(FILES
            "${SCN_EXPORT_HEADER}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/scn"
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
