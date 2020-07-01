find_path(_FMT_INCLUDE_DIRS fmt/format.h)

mark_as_advanced(VAR _FMT_INCLUDE_DIRS)
find_package_handle_standard_args(FMT DEFAULT_MSG _FMT_INCLUDE_DIRS)

add_library(fmt INTERFACE IMPORTED)
set_target_properties(fmt PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${_FMT_INCLUDE_DIRS}
        INTERFACE_COMPILE_DEFINITIONS FMT_HEADER_ONLY=1)
