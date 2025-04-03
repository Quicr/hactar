idf_component_register()

add_subdirectory(lib)
target_link_libraries(${COMPONENT_LIB} INTERFACE nlohmann_json)
