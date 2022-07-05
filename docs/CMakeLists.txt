if (NOT SCN_DOCS)
    return()
endif()

find_package(Doxygen)
if (NOT DOXYGEN_FOUND)
    message(STATUS "Target 'scn_doc' disabled for scnlib (doxygen required)")
    return()
endif()

get_target_property(SCN_PUBLIC_HEADER_DIR scn INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE SCN_PUBLIC_HEADERS ${SCN_PUBLIC_HEADER_DIR}/*.h)

set(DOXYGEN_INPUT_DIR "${PROJECT_SOURCE_DIR}/include")
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
set(DOXYGEN_INDEX_FILE "${DOXYGEN_OUTPUT_FILE}/xml/index.xml")
set(DOXYFILE_IN "${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in")
set(DOXYFILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
        DEPENDS ${SCN_PUBLIC_HEADERS}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "Generating docs"
        VERBATIM)

add_custom_target(scn_doc DEPENDS ${DOXYGEN_INDEX_FILE})

find_package(Sphinx)
if (NOT SPHINX_FOUND)
    message(STATUS "Target 'scn_doc_sphinx' disabled for scnlib (sphinx required)")
    return()
endif ()

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

file(MAKE_DIRECTORY ${SPHINX_BUILD})
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
        COMMAND
        ${SPHINX_EXECUTABLE} -b html
        -Dbreathe_projects.scnlib=${DOXYGEN_OUTPUT_DIR}/xml
        ${SPHINX_SOURCE} ${SPHINX_BUILD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/guide.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/api.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/migration/migration_2_0.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/rationale/index.rst
        ${CMAKE_CURRENT_SOURCE_DIR}/_static/css/custom.css
        ${DOXYGEN_INDEX_FILE}
        MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
        COMMENT "Generating documentation with Sphinx")

add_custom_target(scn_doc_sphinx DEPENDS ${SPHINX_INDEX_FILE})

if (SCN_INSTALL)
    install(DIRECTORY ${SPHINX_BUILD}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/scn
            OPTIONAL)
endif ()
