set(PACKAGE_NAME "STM32Cryptographic")
set(LIBRARY_NAME "STM32Cryptographic")

# Locate the include directory
find_path(
    STM32Cryptographic_INCLUDE_DIR
    "cmox_crypto.h"
    PATHS
        ${STM32Cryptographic_ROOT_DIR}
        /usr/local/include
        /usr/include
    PATH_SUFFIXES include
)

if(NOT STM32Cryptographic_INCLUDE_DIR)
    message(WARNING "Could not find include dir for ${PACKAGE_NAME}")
else()
    message(STATUS "Found include dir for ${PACKAGE_NAME}: ${STM32Cryptographic_INCLUDE_DIR}")
    include_directories(${STM32Cryptographic_INCLUDE_DIR})
endif()

set(SUPPORTED_CORTICES "CM0_CM0PLUS;CM3;CM33;CM4;CM7")

# Locate the libraries
foreach(STM32_CORTEX ${${PACKAGE_NAME}_FIND_COMPONENTS})
    find_library(
        STM32Cryptographic_${STM32_CORTEX}
        NAMES STM32Cryptographic_${STM32_CORTEX}
        PATHS
            ${STM32Cryptographic_ROOT_DIR}
            /usr/local/lib
            /usr/lib
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(NOT STM32Cryptographic_${STM32_CORTEX})
        message(WARNING "Could not find library: STM32Cryptographic_${STM32_CORTEX}")
    else()
        message(STATUS "Found library for ${PACKAGE_NAME}: STM32Cryptographic_${STM32_CORTEX} at ${STM32Cryptographic_${STM32_CORTEX}}")
    endif()

    list(APPEND STM32Cryptographic_LIBRARY "${STM32Cryptographic_${STM32_CORTEX}}")
endforeach()

# Set the STM32Cryptographic_FOUND variable
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PACKAGE_NAME}
    REQUIRED_VARS
        STM32Cryptographic_INCLUDE_DIR
        STM32Cryptographic_LIBRARY
)

# Set output variables
if(${PACKAGE_NAME}_FOUND)
    set(STM32Cryptographic_INCLUDE_DIRS ${STM32Cryptographic_INCLUDE_DIR})
    set(STM32Cryptographic_LIBRARIES ${STM32Cryptographic_LIBRARY})
else()
    set(STM32Cryptographic_INCLUDE_DIRS "")
    set(STM32Cryptographic_LIBRARIES "")
endif()

mark_as_advanced(
    STM32Cryptographic_INCLUDE_DIR
    STM32Cryptographic_LIBRARY
)

if (STM32Cryptographic_FOUND)
    foreach(STM32_CORTEX ${${PACKAGE_NAME}_FIND_COMPONENTS})
        if(NOT TARGET ${PACKAGE_NAME}::${STM32_CORTEX} AND EXISTS "${${PACKAGE_NAME}_LIBRARY}")
            add_library(${PACKAGE_NAME}::${STM32_CORTEX} UNKNOWN IMPORTED)
            set_target_properties(${PACKAGE_NAME}::${STM32_CORTEX} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${${PACKAGE_NAME}_INCLUDE_DIR}"
            )
            if(EXISTS "${${PACKAGE_NAME}_LIBRARY}")
                set_target_properties(${PACKAGE_NAME}::${STM32_CORTEX} PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                    IMPORTED_LOCATION "${${PACKAGE_NAME}_LIBRARY}"
                )
            endif()
        endif()
    endforeach()
endif()
