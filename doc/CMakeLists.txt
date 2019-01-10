find_program(DOXYGEN doxygen)
if (NOT DOXYGEN)
    message(STATUS "Target 'doc' disabled (required doxygen)")
    return()
endif ()

get_filename_component(parent_dir ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
add_custom_target(doc
    COMMAND doxygen
    WORKING_DIRECTORY ${parent_dir})

if (SCN_INSTALL)
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/scn
        OPTIONAL)
endif ()
