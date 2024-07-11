project(moctar)

add_executable(moctar moctar.cpp)

add_subdirectory(dependencies)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC quicr mlspp)

target_compile_options(${PROJECT_NAME}
        PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wpedantic -Wextra -Wall>
        $<$<CXX_COMPILER_ID:MSVC>: >)

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF)
