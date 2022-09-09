add_library(dump INTERFACE)
target_sources(dump PRIVATE
    include/dump/dump.hpp)
target_include_directories(dump
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
if(MSVC)
  target_compile_definitions(dump INTERFACE
    "/Zc:preprocessor" # Enable preprocessor conformance mode
  )
endif()
target_compile_features(dump INTERFACE cxx_std_20)
set_target_properties(dump PROPERTIES
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER include/dump/dump.hpp)
#target_link_libraries(dump PUBLIC ...)
add_library(${PROJECT_NAMESPACE}::dump ALIAS dump)

add_subdirectory(tests)

# Install
install(TARGETS dump
  EXPORT ${PROJECT_NAME}Targets
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dump
  )
