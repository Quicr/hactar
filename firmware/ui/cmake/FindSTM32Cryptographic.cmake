set(PACKAGE_NAME "STM32Cryptographic")
set(LIBRARY_NAME "STM32Cryptographic")

if (${PACKAGE_NAME}_FOUND)
    return()
endif()

# Locate the include directory
find_path(
    ${PACKAGE_NAME}_INCLUDE_DIR
    "cmox_crypto.h"
    PATHS
        ${${PACKAGE_NAME}_ROOT_DIR}
        /usr/local/include
        /usr/include
    PATH_SUFFIXES include
)

if(NOT ${PACKAGE_NAME}_INCLUDE_DIR)
    message(WARNING "Could not find include dir for ${PACKAGE_NAME}")
else()
    message(STATUS "Found include dir for ${PACKAGE_NAME}: ${${PACKAGE_NAME}_INCLUDE_DIR}")
    include_directories(${${PACKAGE_NAME}_INCLUDE_DIR})
endif()

set(SUPPORTED_CORTICES "CM0_CM0PLUS;CM3;CM33;CM4;CM7")

# Locate the libraries
foreach(STM32_CORTEX ${${PACKAGE_NAME}_FIND_COMPONENTS})
    find_library(
        ${PACKAGE_NAME}_${STM32_CORTEX}
        NAMES ${PACKAGE_NAME}_${STM32_CORTEX}
        PATHS
            ${${PACKAGE_NAME}_ROOT_DIR}
            /usr/local/lib
            /usr/lib
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(NOT ${PACKAGE_NAME}_${STM32_CORTEX})
        message(WARNING "Could not find library: ${PACKAGE_NAME}_${STM32_CORTEX}")
    else()
        message(STATUS "Found library for ${PACKAGE_NAME}: ${PACKAGE_NAME}_${STM32_CORTEX} at ${${PACKAGE_NAME}_${STM32_CORTEX}}")
    endif()

    list(APPEND ${PACKAGE_NAME}_LIBRARY "${${PACKAGE_NAME}_${STM32_CORTEX}}")
endforeach()

# Set the ${PACKAGE_NAME}_FOUND variable
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PACKAGE_NAME}
    REQUIRED_VARS
        ${PACKAGE_NAME}_INCLUDE_DIR
        ${PACKAGE_NAME}_LIBRARY
)

# Set output variables
if(${PACKAGE_NAME}_FOUND)
    set(${PACKAGE_NAME}_INCLUDE_DIRS ${${PACKAGE_NAME}_INCLUDE_DIR})
    set(${PACKAGE_NAME}_LIBRARIES ${${PACKAGE_NAME}_LIBRARY})
else()
    set(${PACKAGE_NAME}_INCLUDE_DIRS "")
    set(${PACKAGE_NAME}_LIBRARIES "")
endif()

mark_as_advanced(
    ${PACKAGE_NAME}_INCLUDE_DIR
    ${PACKAGE_NAME}_LIBRARY
)

if (${PACKAGE_NAME}_FOUND)
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
