add_executable(app)
target_sources(app
  PRIVATE
    src/main.cpp)
target_compile_features(app PRIVATE cxx_std_17)
set_target_properties(app PROPERTIES
  VERSION ${PROJECT_VERSION})
# note: macOS is APPLE and also UNIX !
if(APPLE)
  set_target_properties(app PROPERTIES
    INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX AND NOT APPLE)
  set_target_properties(app PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
target_link_libraries(app PRIVATE
  ${PROJECT_NAMESPACE}::dump)
add_executable(${PROJECT_NAMESPACE}::app ALIAS app)

if(BUILD_TESTING)
  add_test(NAME cpp_app_test COMMAND app)
endif()

# Install
include(GNUInstallDirs)
install(TARGETS app
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
