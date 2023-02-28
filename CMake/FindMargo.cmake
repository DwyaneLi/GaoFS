find_path(MARGO_INCLUDE_DIR
        NAMES margo.h
        )

find_library(MARGO_LIBRARY
        NAMES margo
        )

set(MARGO_INCLUDE_DIRS ${MARGO_INCLUDE_DIR})
set(MARGO_LIBRARIES ${MARGO_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Margo DEFAULT_MSG MARGO_LIBRARY MARGO_INCLUDE_DIR)

mark_as_advanced(
        MARGO_LIBRARY
        MARGO_INCLUDE_DIR
)