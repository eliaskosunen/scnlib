if (NOT SCN_DOCS)
    return()
endif()

find_package(Doxygen QUIET)
if (NOT DOXYGEN_FOUND)
    message(STATUS "Target 'scn_docs' disabled for scnlib (doxygen required)")
    return()
endif()

find_package(poxy QUIET)
if (NOT POXY_FOUND)
    message(STATUS "Target 'scn_docs' disabled for scnlib (poxy required)")
    return()
endif()

set(SCN_ABSOLUTE_PUBLIC_HEADERS "${SCN_PUBLIC_HEADERS}")
list(TRANSFORM SCN_ABSOLUTE_PUBLIC_HEADERS PREPEND "${CMAKE_CURRENT_LIST_DIR}/../")

add_custom_command(
        COMMAND ${POXY_EXECUTABLE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT "${CMAKE_CURRENT_LIST_DIR}/html"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_LIST_DIR}/poxy.toml"
        DEPENDS
            "${CMAKE_CURRENT_LIST_DIR}/pages/faq.md"
            "${CMAKE_CURRENT_LIST_DIR}/pages/guide.md"
            "${CMAKE_CURRENT_LIST_DIR}/pages/mainpage.md"
            "${CMAKE_CURRENT_LIST_DIR}/../CHANGELOG.md"
            "${CMAKE_CURRENT_LIST_DIR}/script/monospace-headers.js"
            "${SCN_ABSOLUTE_PUBLIC_HEADERS}"
        COMMENT "Generating docs"
        VERBATIM)
add_custom_target(scn_docs DEPENDS "${CMAKE_CURRENT_LIST_DIR}/html")
