find_program(POXY_EXECUTABLE
        NAMES poxy
        DOC "Path to poxy executable")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(poxy "Failed to find poxy executable" POXY_EXECUTABLE)
