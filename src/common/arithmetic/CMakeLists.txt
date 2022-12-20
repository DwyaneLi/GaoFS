add_library(arithmetic INTERFACE)
target_sources(arithmetic
        INTERFACE
        ${INCLUDE_DIR}/common/arithmetic/arithmetic.hpp
        )

target_include_directories(arithmetic
        INTERFACE
        ${INCLUDE_DIR}/common/arithmetic/
        )

#target_code_coverage(arithmetic INTERFACE)