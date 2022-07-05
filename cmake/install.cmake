if (NOT SCN_INSTALL)
    return()
endif()

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/scn)

install(TARGETS
            scn
            simdutf fast_float
        EXPORT scn-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT scn-targets
        FILE
            scnTargets.cmake
        NAMESPACE
            scn::
        DESTINATION
            "${INSTALL_CONFIGDIR}"
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/scnConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
        INSTALL_DESTINATION "${INSTALL_CONFIGDIR}"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
        DESTINATION "${INSTALL_CONFIGDIR}"
)

export(EXPORT scn-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/scnTargets.cmake"
        NAMESPACE scn::
)

export(PACKAGE scn)
















